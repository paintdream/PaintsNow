#include "BridgeSunset.h"
#include "../../Core/System/ConsoleStream.h"
using namespace PaintsNow;

SharedContext::SharedContext() : next(nullptr) {
	atomicValue.store(0, std::memory_order_release);
}

SharedContext::~SharedContext() {
	SharedContext* p = next.exchange(nullptr, std::memory_order_acq_rel);
	while (p != nullptr) {
		SharedContext* t = p;
		p = p->next.load(std::memory_order_relaxed);

		t->next = nullptr;
		t->ReleaseObject();
	}
}

BridgeSunset::BridgeSunset(IThread& t, IScript& s, uint32_t threadCount, uint32_t warpCount, long balancer) : ISyncObject(t), RequestPool(s, threadCount * 2), threadPool(t, threadCount, balancer), kernel(threadPool, warpCount) {
	exiting.store(0, std::memory_order_relaxed);
	logErrorStream = new ConsoleStream(stderr);
	logInfoStream = new ConsoleStream(stdout);

	LogInfo().Printf("[BridgeSunset] Initialize thread pool...\n");

	memset(warpBitset, 0, sizeof(warpBitset));

	if (!threadPool.IsInitialized()) {
		threadPool.Initialize();
	}

	script.DoLock();
	for (uint32_t k = 0; k < threadPool.GetThreadCount(); k++) {
		threadPool.SetThreadContext(k, this);
	}

	script.UnLock();
}

void BridgeSunset::Reset() {
	exiting.store(1, std::memory_order_release);
	kernel.Reset();
	requestPool.Clear();
}

bool BridgeSunset::IsExiting() const {
	return exiting.load(std::memory_order_acquire) != 0;
}

BridgeSunset::~BridgeSunset() {
	assert(threadPool.IsInitialized());
	threadPool.Uninitialize();
	logErrorStream->Destroy();
	logInfoStream->Destroy();
}

IStreamBase& BridgeSunset::LogError() {
	assert(logErrorStream != nullptr);
	return *logErrorStream;
}

IStreamBase& BridgeSunset::LogInfo() {
	assert(logInfoStream != nullptr);
	return *logInfoStream;
}

void BridgeSunset::ScriptInitialize(IScript::Request& request) {
	Library::ScriptInitialize(request);
}

void BridgeSunset::ScriptUninitialize(IScript::Request& request) {
	Library::ScriptUninitialize(request);
}

TObject<IReflect>& BridgeSunset::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewSharedContext)[ScriptMethod = "NewSharedContext"];
		ReflectMethod(RequestSetSharedContextCounter)[ScriptMethodLocked = "SetSharedContextCounter"];
		ReflectMethod(RequestCompareExchangeSharedContextCounter)[ScriptMethodLocked = "CompareExchangeSharedContextCounter"];
		ReflectMethod(RequestAddSharedContextCounter)[ScriptMethodLocked = "AddSharedContextCounter"];
		ReflectMethod(RequestSubSharedContextCounter)[ScriptMethodLocked = "SubSharedContextCounter"];
		ReflectMethod(RequestGetSharedContextCounter)[ScriptMethodLocked = "GetSharedContextCounter"];
		ReflectMethod(RequestSetSharedContextObjects)[ScriptMethodLocked = "SetSharedContextObjects"];
		ReflectMethod(RequestGetSharedContextObjects)[ScriptMethodLocked = "GetSharedContextObjects"];
		ReflectMethod(RequestSetSharedContextObject)[ScriptMethodLocked = "SetSharedContextObject"];
		ReflectMethod(RequestGetSharedContextObject)[ScriptMethodLocked = "GetSharedContextObject"];
		ReflectMethod(RequestChainSharedContext)[ScriptMethodLocked = "ChainSharedContext"];
		ReflectMethod(RequestExtractSharedContextChain)[ScriptMethodLocked = "ExtractSharedContextChain"];

		ReflectMethod(RequestNewGraph)[ScriptMethod = "NewGraph"];
		ReflectMethod(RequestQueueGraphRoutine)[ScriptMethodLocked = "QueueGraphRoutine"];
		ReflectMethod(RequestConnectGraphRoutine)[ScriptMethodLocked = "ConnectGraphRoutine"];
		ReflectMethod(RequestExecuteGraph)[ScriptMethod = "ExecuteGraph"];
		ReflectMethod(RequestQueueRoutine)[ScriptMethodLocked = "QueueRoutine"]; // Auto-locked
		ReflectMethod(RequestGetWarpCount)[ScriptMethodLocked = "GetWarpCount"];
		ReflectMethod(RequestSetWarpIndex)[ScriptMethodLocked = "SetWarpIndex"];
		ReflectMethod(RequestGetWarpIndex)[ScriptMethodLocked = "GetWarpIndex"];
		ReflectMethod(RequestGetCurrentThreadIndex)[ScriptMethodLocked = "GetCurrentThreadIndex"];
		ReflectMethod(RequestGetCurrentWarpIndex)[ScriptMethodLocked = "GetCurrentWarpIndex"];
		ReflectMethod(RequestGetNullWarpIndex)[ScriptMethodLocked = "GetNullWarpIndex"];
		ReflectMethod(RequestAllocateWarpIndex)[ScriptMethodLocked = "AllocateWarpIndex"];
		ReflectMethod(RequestFreeWarpIndex)[ScriptMethodLocked = "FreeWarpIndex"];
		ReflectMethod(RequestSetWarpPriority)[ScriptMethodLocked = "SetWarpPriority"];
		ReflectMethod(RequestPin)[ScriptMethodLocked = "Pin"];
		ReflectMethod(RequestUnpin)[ScriptMethodLocked = "Unpin"];
		ReflectMethod(RequestClone)[ScriptMethod = "Clone"];
	}

	return *this;
}

Kernel& BridgeSunset::GetKernel() {
	return kernel;
}

ThreadPool& BridgeSunset::GetThreadPool() {
	return threadPool;
}

void BridgeSunset::RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(unit);


	GetKernel().QueueRoutinePost(unit.Get(), CreateTaskScriptOnce(callback));
}

uint32_t BridgeSunset::RequestGetWarpCount(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return GetKernel().GetWarpCount();
}

TShared<TaskGraph> BridgeSunset::RequestNewGraph(IScript::Request& request, int32_t startupWarp) {
	CHECK_REFERENCES_NONE();
	return TShared<TaskGraph>::From(new TaskGraph(kernel));
}

int32_t BridgeSunset::RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(graph);
	CHECK_DELEGATE(unit);

	return verify_cast<int32_t>(graph->Insert(unit.Get(), CreateTaskScriptOnce(callback)));
}

void BridgeSunset::RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, int32_t prev, int32_t next) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Next(prev, next);
}

class ScriptTaskWrapper {
public:
	ScriptTaskWrapper(BridgeSunset* b, ITask* t) : bridgeSunset(b), task(t) {}

	void operator () () {
		bridgeSunset->GetThreadPool().Dispatch(task);
	}

	BridgeSunset* bridgeSunset;
	ITask* task;
};

void BridgeSunset::RequestExecuteGraph(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Request::Ref callback) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	if (callback) {
		ScriptTaskWrapper wrapper(this, CreateTaskScriptOnce(callback));
		graph->Dispatch(WrapClosure(std::move(wrapper), &ScriptTaskWrapper::operator ()));
	} else {
		graph->Dispatch();
	}
}

void BridgeSunset::RequestSetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source, uint32_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->SetWarpIndex(index);
}

uint32_t BridgeSunset::RequestGetWarpIndex(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	return source->GetWarpIndex();
}

uint32_t BridgeSunset::RequestGetCurrentThreadIndex(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	return kernel.GetThreadPool().GetCurrentThreadIndex();
}

uint32_t BridgeSunset::RequestGetCurrentWarpIndex(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	return kernel.GetCurrentWarpIndex();
}

uint32_t BridgeSunset::RequestGetNullWarpIndex(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	return ~(uint32_t)0;
}

TShared<SharedTiny> BridgeSunset::RequestClone(IScript::Request& request, IScript::Delegate<SharedTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	return static_cast<SharedTiny*>(source->Clone());
}

void BridgeSunset::RequestPin(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->Flag().fetch_or(Tiny::TINY_PINNED);
}

void BridgeSunset::RequestUnpin(IScript::Request& request, IScript::Delegate<WarpTiny> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);

	source->Flag().fetch_and(~Tiny::TINY_PINNED);
}

uint32_t BridgeSunset::AllocateWarpIndex() {
	static_assert(WARP_VAR_COUNT * sizeof(size_t) * 8 == (1 << WarpTiny::WARP_BITS), "Warp count must be multiplier of sizeof(size_t) * 8");

	uint32_t count = kernel.GetWarpCount();
	for (uint32_t i = 0; i < count; i++) {
		std::atomic<size_t>& v = reinterpret_cast<std::atomic<size_t>&>(warpBitset[i]);
		size_t value = v.load(std::memory_order_relaxed);
		if (value != ~(size_t)0) {
			size_t mask = Math::Alignment(value + 1);
			if (!(v.fetch_or(mask, std::memory_order_relaxed) & mask)) {
				return (1 + Math::Log2x(mask)) & ((1 << WarpTiny::WARP_BITS) - 1); // warp 0 is reserved
			}
		}
	}

	// overflow
	return 0;
}

uint32_t BridgeSunset::RequestAllocateWarpIndex(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return AllocateWarpIndex();
}

void BridgeSunset::FreeWarpIndex(uint32_t warpIndex) {
	if (warpIndex > 0) {
		std::atomic<size_t>& v = reinterpret_cast<std::atomic<size_t>&>(warpBitset[warpIndex / (sizeof(size_t) * 8)]);
		size_t mask = (size_t)1 << (warpIndex & (sizeof(size_t) * 8 - 1));
		v.fetch_and(~mask, std::memory_order_relaxed);
	}
}

void BridgeSunset::RequestFreeWarpIndex(IScript::Request& request, uint32_t warpIndex) {
	CHECK_REFERENCES_NONE();

	if (warpIndex != 0) {
		FreeWarpIndex(warpIndex);
	}  else {
		request.Error("Can not free warp 0");
	}
}

void BridgeSunset::RequestSetWarpPriority(IScript::Request& request, uint32_t warpIndex, uint32_t priority) {
	CHECK_REFERENCES_NONE();
	if (warpIndex < kernel.GetWarpCount()) {
		kernel.SetWarpPriority(warpIndex, priority);
	}
}

TShared<SharedContext> BridgeSunset::RequestNewSharedContext(IScript::Request& request) {
	return TShared<SharedContext>::From(new SharedContext());
}

size_t BridgeSunset::RequestSetSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->atomicValue.exchange(counter, std::memory_order_acq_rel);
}

bool BridgeSunset::RequestCompareExchangeSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t comparedTo, size_t counter) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->atomicValue.compare_exchange_strong(comparedTo, counter, std::memory_order_acq_rel);
}

size_t BridgeSunset::RequestAddSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->atomicValue.fetch_add(counter, std::memory_order_acq_rel);
}

size_t BridgeSunset::RequestSubSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t counter) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->atomicValue.fetch_sub(counter, std::memory_order_acq_rel);
}

size_t BridgeSunset::RequestGetSharedContextCounter(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->atomicValue.load(std::memory_order_acquire);
}

void BridgeSunset::RequestSetSharedContextObjects(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, std::vector<IScript::Delegate<SharedTiny> >& tinies) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	std::vector<TShared<SharedTiny> > vec(tinies.size());
	for (size_t i = 0; i < tinies.size(); i++) {
		vec[i] = tinies[i].Get();
	}

	std::swap(sharedContext->objectVector, vec);
}

void BridgeSunset::RequestSetSharedContextObject(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t index, IScript::Delegate<SharedTiny> tiny) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	if (index < sharedContext->objectVector.size()) {
		sharedContext->objectVector[index] = tiny.Get();
	}
}

const std::vector<TShared<SharedTiny> >& BridgeSunset::RequestGetSharedContextObjects(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	return sharedContext->objectVector;
}

TShared<SharedTiny> BridgeSunset::RequestGetSharedContextObject(IScript::Request& request, IScript::Delegate<SharedContext> sharedContext, size_t index) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sharedContext);

	if (index < sharedContext->objectVector.size()) {
		return sharedContext->objectVector[index];
	} else {
		return nullptr;
	}
}

void BridgeSunset::RequestChainSharedContext(IScript::Request& request, IScript::Delegate<SharedContext> sentinelSharedContext, IScript::Delegate<SharedContext> sharedContext) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sentinelSharedContext);
	CHECK_DELEGATE(sharedContext);

	SharedContext* p = sentinelSharedContext.Get();
	SharedContext* q = sharedContext.Get();
	q->ReferenceObject();

	SharedContext* head = q->next.load(std::memory_order_relaxed);
	do {
		q->next.store(head, std::memory_order_relaxed);
	} while (!p->next.compare_exchange_weak(head, q, std::memory_order_release, std::memory_order_relaxed));
}

std::vector<TShared<SharedContext> > BridgeSunset::RequestExtractSharedContextChain(IScript::Request& request, IScript::Delegate<SharedContext> sentinelSharedContext) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(sentinelSharedContext);
	
	std::vector<TShared<SharedContext> > result;
	SharedContext* p = sentinelSharedContext->next.exchange(nullptr, std::memory_order_acq_rel);
	while (p != nullptr) {
		SharedContext* t = p;
		p = p->next.load(std::memory_order_relaxed);

		t->next = nullptr;
		result.emplace_back(TShared<SharedContext>::From(t));
	}

	return result;
}

