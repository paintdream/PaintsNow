#include "BridgeSunset.h"

using namespace PaintsNow;

BridgeSunset::BridgeSunset(IThread& t, IScript& s, uint32_t threadCount, uint32_t warpCount) : ISyncObject(t), RequestPool(s, warpCount), threadPool(t, threadCount), kernel(threadPool, warpCount) {
	memset(warpBitset, 0, sizeof(warpBitset));

	if (!threadPool.IsInitialized()) {
		threadPool.Initialize();
	}

	script.DoLock();
	for (uint32_t k = 0; k < threadPool.GetThreadCount(); k++) {
		threadPool.SetThreadContext(k, this);
	}

	// register script dispatcher hook
	origDispatcher = script.GetDispatcher();
	script.SetDispatcher(Wrap(this, &BridgeSunset::ContinueScriptDispatcher));
	script.UnLock();
}

BridgeSunset::~BridgeSunset() {
	assert(threadPool.IsInitialized());
	threadPool.Uninitialize();

	IScript::Request& mainRequest = script.GetDefaultRequest();
	assert(script.GetDispatcher() == Wrap(this, &BridgeSunset::ContinueScriptDispatcher));
	script.SetDispatcher(origDispatcher);
	requestPool.Clear(); // clear request pool
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
		ReflectMethod(RequestNewGraph)[ScriptMethod = "NewGraph"];
		ReflectMethod(RequestQueueGraphRoutine)[ScriptMethodLocked = "QueueGraphRoutine"];
		ReflectMethod(RequestConnectGraphRoutine)[ScriptMethodLocked = "ConnectGraphRoutine"];
		ReflectMethod(RequestExecuteGraph)[ScriptMethod = "ExecuteGraph"];
		ReflectMethod(RequestQueueRoutine)[ScriptMethodLocked = "QueueRoutine"]; // Auto-locked
		ReflectMethod(RequestGetWarpCount)[ScriptMethodLocked = "GetWarpCount"];
		ReflectMethod(RequestSetWarpIndex)[ScriptMethodLocked = "SetWarpIndex"];
		ReflectMethod(RequestGetWarpIndex)[ScriptMethodLocked = "GetWarpIndex"];
		ReflectMethod(RequestGetCurrentWarpIndex)[ScriptMethodLocked = "GetCurrentWarpIndex"];
		ReflectMethod(RequestGetNullWarpIndex)[ScriptMethodLocked = "GetNullWarpIndex"];
		ReflectMethod(RequestAllocateWarpIndex)[ScriptMethodLocked = "AllocateWarpIndex"];
		ReflectMethod(RequestFreeWarpIndex)[ScriptMethodLocked = "FreeWarpIndex"];
		ReflectMethod(RequestPin)[ScriptMethodLocked = "Pin"];
		ReflectMethod(RequestUnpin)[ScriptMethodLocked = "Unpin"];
		ReflectMethod(RequestClone)[ScriptMethod = "Clone"];
	}

	return *this;
}

Kernel& BridgeSunset::GetKernel() {
	return kernel;
}

void BridgeSunset::ContinueScriptDispatcher(IScript::Request& request, IHost* host, size_t paramCount, const TWrapper<void, IScript::Request&>& continuer) {
	// check if current warp is yielded
	static thread_local uint32_t stackWarpIndex = ~(uint32_t)0;
	uint32_t warpIndex = kernel.GetCurrentWarpIndex();
	if (warpIndex != ~(uint32_t)0) {
		stackWarpIndex = warpIndex;
		continuer(request);
		stackWarpIndex = warpIndex;
	} else {
		uint32_t saveStackWarpIndex = stackWarpIndex;
		if (saveStackWarpIndex != ~(uint32_t)0) {
			request.UnLock();
			kernel.WaitWarp(stackWarpIndex);
			request.DoLock();
		}

		continuer(request);
		stackWarpIndex = saveStackWarpIndex;
		kernel.YieldCurrentWarp();
	}
}

void BridgeSunset::RequestQueueRoutine(IScript::Request& request, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(unit);

	if (GetKernel().GetCurrentWarpIndex() != unit->GetWarpIndex()) {
		GetKernel().QueueRoutinePost(unit.Get(), CreateTaskScriptOnce(callback));
	} else {
		// Already locked!
		// request.DoLock();
		request.Push();
		request.Call(callback);
		request.Dereference(callback);
		request.Pop();
		// request.UnLock();
	}
}

uint32_t BridgeSunset::RequestGetWarpCount(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return GetKernel().GetWarpCount();
}

TShared<TaskGraph> BridgeSunset::RequestNewGraph(IScript::Request& request, int32_t startupWarp) {
	CHECK_REFERENCES_NONE();
	return TShared<TaskGraph>::From(new TaskGraph(kernel));
}

size_t BridgeSunset::RequestQueueGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, IScript::Delegate<WarpTiny> unit, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(graph);
	CHECK_DELEGATE(unit);

	return graph->Insert(unit.Get(), CreateTaskScriptOnce(callback));
}

void BridgeSunset::RequestConnectGraphRoutine(IScript::Request& request, IScript::Delegate<TaskGraph> graph, int32_t prev, int32_t next) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(graph);

	graph->Next(prev, next);
}

struct ScriptTaskWrapper {
	ScriptTaskWrapper(BridgeSunset* b, ITask* t) : bridgeSunset(b), task(t) {}

	void operator () () {
		bridgeSunset->threadPool.Dispatch(task);
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

uint32_t BridgeSunset::RequestAllocateWarpIndex(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	static_assert(WARP_VAR_COUNT * sizeof(size_t) * 8 == (1 << WarpTiny::WARP_BITS), "Warp count must be multiplier of sizeof(size_t) * 8");

	for (size_t i = 0; i < WARP_VAR_COUNT; i++) {
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

void BridgeSunset::RequestFreeWarpIndex(IScript::Request& request, uint32_t warpIndex) {
	CHECK_REFERENCES_NONE();

	if (warpIndex-- != 0) {
		std::atomic<size_t>& v = reinterpret_cast<std::atomic<size_t>&>(warpBitset[warpIndex / (sizeof(size_t) * 8)]);
		size_t mask = (size_t)1 << (warpIndex & (sizeof(size_t) * 8 - 1));
		v.fetch_and(~mask, std::memory_order_relaxed);
	} else {
		request.Error("Can not free warp 0");
	}
}
