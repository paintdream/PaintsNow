#include "CrossScript.h"
#include "../Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

CrossRoutine::CrossRoutine(IScript::RequestPool* p, IScript::Request::Ref r) : pool(p), ref(r) {}

CrossRoutine::~CrossRoutine() {
	Clear();
}

void CrossRoutine::Clear() {
	if (ref) {
		IScript::Request& req = pool->GetScript().GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		ref.value = 0;
		req.UnLock();
	}
}

void CrossRoutine::ScriptUninitialize(IScript::Request& request) {
	SharedTiny::ScriptUninitialize(request);
}

static void SysCall(IScript::Request& request, IScript::Delegate<CrossRoutine> routine, IScript::Request::Arguments& args) {
	CrossScript* crossComponent = static_cast<CrossScript*>(routine->pool);
	assert(crossComponent != nullptr);

	crossComponent->Call(request, routine.Get(), args);
}

static void SysCallAsync(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<CrossRoutine> routine, IScript::Request::Arguments& args) {
	CrossScript* crossComponent = static_cast<CrossScript*>(routine->pool);
	assert(crossComponent != nullptr);

	crossComponent->CallAsync(request, callback, routine.Get(), args);
}

CrossScript::CrossScript(ThreadPool& e, IScript& script) : RequestPool(script, e.GetThreadCount()), threadPool(e) {
	script.DoLock();
	IScript::Request& request = script.GetDefaultRequest();
	request << global << key("io") << nil << endtable;
	request << global << key("os") << nil << endtable;
	request << global << key("SysCall") << request.Adapt(Wrap(SysCall));
	request << global << key("SysCallAsync") << request.Adapt(Wrap(SysCallAsync));
	script.SetErrorHandler(Wrap(this, &CrossScript::ErrorHandler));
	script.UnLock();
}

void CrossScript::ErrorHandler(IScript::Request& request, const String& err) {
	fprintf(stderr, "[CrossScript] subscript error: %s\n", err.c_str());
}

CrossScript::~CrossScript() {
	requestPool.Clear();
	script.ReleaseDevice();
}

void CrossScript::ScriptUninitialize(IScript::Request& request) {
	// in case of recursive locking
	request.UnLock();
	BaseClass::ScriptUninitialize(request);
	request.DoLock();
}

TObject<IReflect>& CrossScript::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {}

	return *this;
}

class RoutineWrapper {
public:
	RoutineWrapper(rvalue<TShared<CrossRoutine> > r) : routine(std::move(r)) {}

	~RoutineWrapper() {}
	void operator () (IScript::Request& request, IScript::Request::Arguments& args) {
		// always use sync call
		CrossScript* crossComponent = static_cast<CrossScript*>(routine->pool);
		assert(crossComponent != nullptr);

		crossComponent->Call(request, routine(), args);
	}

	TShared<CrossRoutine> routine;
};

TShared<CrossRoutine> CrossScript::Get(const String& name) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref;
	request << global;
	request << key(name) >> ref;
	request << endtable;
	request.UnLock();

	if (ref.value != 0) {
		return TShared<CrossRoutine>::From(new CrossRoutine(this, ref));
	} else {
		return nullptr;
	}
}

TShared<CrossRoutine> CrossScript::Load(const String& code) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "CrossScript");
	request.UnLock();

	if (ref.value != 0) {
		return TShared<CrossRoutine>::From(new CrossRoutine(this, ref));
	} else {
		return nullptr;
	}
}

static void CopyTable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyArray(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest);
static void CopyVariable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest, IScript::Request::TYPE type) {
	switch (type) {
		case IScript::Request::NIL:
			request << nil;
			break;
		case IScript::Request::BOOLEAN:
		{
			bool value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::NUMBER:
		{
			double value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::INTEGER:
		{
			int64_t value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::STRING:
		{
			String value;
			fromRequest >> value;
			request << value;
			break;
		}
		case IScript::Request::TABLE:
		{
			CopyTable(flag, request, fromRequest);
			break;
		}
		case IScript::Request::ARRAY:
		{
			CopyArray(flag, request, fromRequest);
			break;
		}
		case IScript::Request::FUNCTION:
		{
			// convert to reference
			IScript::Request::Ref ref;
			fromRequest >> ref;
			IScript::Request::AutoWrapperBase* wrapper = fromRequest.GetWrapper(ref);
			if (wrapper != nullptr) {
				// Native function
				request << *wrapper;
				fromRequest.Dereference(ref);
			} else {
				// managed by remote routine
				TShared<CrossRoutine> remoteRoutine = TShared<CrossRoutine>::From(new CrossRoutine(fromRequest.GetRequestPool(), ref));
				if (flag & CrossScript::CROSSSCRIPT_TRANSPARENT) {
					// create wrapper
					RoutineWrapper routineWrapper(std::move(remoteRoutine));
					request << request.Adapt(WrapClosure(std::move(routineWrapper), &RoutineWrapper::operator ()));
				} else {
					request << remoteRoutine;
				}
			}

			break;
		}
		case IScript::Request::OBJECT:
		{
			IScript::BaseDelegate d;
			fromRequest >> d;
			IScript::Object* object = d.Get();
			if (flag & CrossScript::CROSSSCRIPT_TRANSPARENT) {
				CrossRoutine* routine = object->QueryInterface(UniqueType<CrossRoutine>());
				if (routine != nullptr) {
					if (&routine->pool->GetScript() == request.GetScript()) {
						request << routine->ref;
					} else {
						TShared<CrossRoutine> r(routine);
						RoutineWrapper routineWrapper(std::move(r));
						request << request.Adapt(WrapClosure(std::move(routineWrapper), &RoutineWrapper::operator ()));
					}
				} else {
					request << object;
				}
			} else {
				request << object;
			}

			break;
		}
		case IScript::Request::ANY:
		{
			// omitted.
			break;
		}
	}
}

static void CopyArray(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::ArrayStart ts;
	ts.count = 0;
	fromRequest >> ts;
	request << beginarray;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(flag, request, fromRequest, fromRequest.GetCurrentType());
	}

	IScript::Request::Iterator it;
	while (true) {
		fromRequest >> it;
		if (!it) break;

		request << key;
		CopyVariable(flag, request, fromRequest, it.keyType);
		CopyVariable(flag, request, fromRequest, it.valueType);
	}

	request << endarray;
	fromRequest << endarray;
}

static void CopyTable(uint32_t flag, IScript::Request& request, IScript::Request& fromRequest) {
	IScript::Request::TableStart ts;
	ts.count = 0;
	fromRequest >> ts;
	request << begintable;
	for (size_t j = 0; j < ts.count; j++) {
		CopyVariable(flag, request, fromRequest, fromRequest.GetCurrentType());
	}

	IScript::Request::Iterator it;
	while (true) {
		fromRequest >> it;
		if (!it) break;

		request << key;
		CopyVariable(flag, request, fromRequest, it.keyType);
		CopyVariable(flag, request, fromRequest, it.valueType);
	}

	request << endtable;
	fromRequest << endtable;
}

void CrossScript::Call(IScript::Request& fromRequest, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	// self call
	bool selfCall = fromRequest.GetRequestPool() == remoteRoutine->pool;
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		toRequest.DoLock();
		if (!selfCall)
			fromRequest.DoLock();

		toRequest.Push();
		uint32_t flag = selfCall ? 0 : Flag().load(std::memory_order_relaxed);
		// read remaining parameters
		for (size_t i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}

		if (!selfCall)
			fromRequest.UnLock();

		toRequest.Call(remoteRoutine->ref);

		if (!selfCall)
			fromRequest.DoLock();

		for (int k = 0; k < toRequest.GetCount(); k++) {
			CopyVariable(flag, fromRequest, toRequest, toRequest.GetCurrentType());
		}

		toRequest.Pop();
		if (!selfCall)
			fromRequest.UnLock();
		toRequest.UnLock();

		requestPool.ReleaseSafe(&toRequest);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

void CrossScript::ExecuteCall(IScript::RequestPool* returnPool, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine) {
	OPTICK_EVENT();

	toRequest.DoLock();
	toRequest.Call(remoteRoutine->ref);
	IScript::RequestPool* pool = toRequest.GetRequestPool();
	toRequest.UnLock();

	Dispatch(pool, returnPool, CreateTaskContextFree(Wrap(this, &CrossScript::CompleteCall), returnPool, std::ref(toRequest), callback, remoteRoutine));
}

void CrossScript::CompleteCall(IScript::RequestPool* returnPool, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine) {
	OPTICK_EVENT();

	IScript::Request& returnRequest = *returnPool->requestPool.AcquireSafe();
	returnRequest.DoLock();
	returnRequest.Push();

	uint32_t flag = Flag().load(std::memory_order_relaxed);
	toRequest.DoLock();
	for (int i = 0; i < toRequest.GetCount(); i++) {
		CopyVariable(flag, returnRequest, toRequest, toRequest.GetCurrentType());
	}

	toRequest.Pop();
	toRequest.UnLock();

	returnRequest.Call(callback);
	returnRequest.Dereference(callback);
	returnRequest.Pop();
	returnRequest.UnLock();

	requestPool.ReleaseSafe(&toRequest);
	returnPool->requestPool.ReleaseSafe(&returnRequest);
}

void CrossScript::PrepareCall(IScript::Request& fromRequest, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	toRequest.Push();
	// read remaining parameters
	uint32_t flag = Flag().load(std::memory_order_relaxed);
	for (size_t i = 0; i < args.count; i++) {
		CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
	}

	IScript::RequestPool* targetPool = toRequest.GetRequestPool();
	toRequest.UnLock();
	IScript::RequestPool* pool = fromRequest.GetRequestPool();
	assert(&pool->GetScript() == fromRequest.GetScript());
	fromRequest.UnLock();

	Dispatch(pool, targetPool, CreateTaskContextFree(Wrap(this, &CrossScript::ExecuteCall), pool, std::ref(toRequest), callback, remoteRoutine));
}

bool CrossScript::IsLocked() const {
	return script.IsLocked();
}

void CrossScript::Dispatch(IScript::RequestPool* fromPool, IScript::RequestPool* toPool, ITask* task) {
	if (&toPool->GetScript() == &script) {
		threadPool.Dispatch(task);
	} else {
		task->Execute(nullptr);
	}
}

bool CrossScript::TryCallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		fromRequest.DoLock();
		if (toRequest.TryLock()) {
			PrepareCall(fromRequest, toRequest, callback, remoteRoutine, args);
			return true;
		} else {
			fromRequest.UnLock();
			return false;
		}
	} else {
		fromRequest.Error("Invalid ref.");
		return false;
	}
}

void CrossScript::CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<CrossRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		fromRequest.DoLock();
		toRequest.DoLock();

		PrepareCall(fromRequest, toRequest, callback, remoteRoutine, args);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

