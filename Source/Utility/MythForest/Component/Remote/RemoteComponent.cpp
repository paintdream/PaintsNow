#include "RemoteComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

static void ErrorHandler(IScript::Request& request, const String& err) {
	fprintf(stderr, "RemoteRoutine subscript error: %s\n", err.c_str());
}

RemoteRoutine::RemoteRoutine(IScript::RequestPool* p, IScript::Request::Ref r) : pool(p), ref(r) {}

RemoteRoutine::~RemoteRoutine() {
	Clear();
}

void RemoteRoutine::Clear() {
	if (ref) {
		IScript::Request& req = pool->GetScript().GetDefaultRequest();

		req.DoLock();
		req.Dereference(ref);
		ref.value = 0;
		req.UnLock();
	}
}

void RemoteRoutine::ScriptUninitialize(IScript::Request& request) {
	SharedTiny::ScriptUninitialize(request);
}

static void SysCall(IScript::Request& request, IScript::Delegate<RemoteRoutine> routine, IScript::Request::Arguments& args) {
	RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
	assert(remoteComponent != nullptr);

	remoteComponent->Call(request, routine.Get(), args);
}

static void SysCallAsync(IScript::Request& request, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> routine, IScript::Request::Arguments& args) {
	RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
	assert(remoteComponent != nullptr);

	remoteComponent->CallAsync(request, callback, routine.Get(), args);
}

RemoteComponent::RemoteComponent(Engine& e) : RequestPool(*e.interfaces.script.NewScript(), e.GetKernel().GetWarpCount()), engine(e) {
	script.DoLock();
	IScript::Request& request = script.GetDefaultRequest();
	request << global << key("io") << nil << endtable;
	request << global << key("os") << nil << endtable;
	request << global << key("SysCall") << request.Adapt(Wrap(SysCall));
	request << global << key("SysCallAsync") << request.Adapt(Wrap(SysCallAsync));
	script.SetErrorHandler(Wrap(ErrorHandler));
	script.UnLock();
}

RemoteComponent::~RemoteComponent() {
	requestPool.Clear();
	script.ReleaseDevice();	
}

TObject<IReflect>& RemoteComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {}

	return *this;
}

struct RoutineWrapper {
#if defined(_MSC_VER) && _MSC_VER <= 1200
	RoutineWrapper(rvalue<TShared<RemoteRoutine> > r) : routine(r) {}
#else
	RoutineWrapper(rvalue<TShared<RemoteRoutine> > r) : routine(std::move(r)) {}
#endif

	~RoutineWrapper() {}
	void operator () (IScript::Request& request, IScript::Request::Arguments& args) {
		// always use sync call
		RemoteComponent* remoteComponent = static_cast<RemoteComponent*>(routine->pool);
		assert(remoteComponent != nullptr);

		remoteComponent->Call(request, routine(), args);
	}

	TShared<RemoteRoutine> routine;
};

TShared<RemoteRoutine> RemoteComponent::Get(const String& name) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref;
	request << global;
	request << key(name) >> ref;
	request << endtable;
	request.UnLock();

	if (ref.value != 0) {
		return TShared<RemoteRoutine>::From(new RemoteRoutine(this, ref));
	} else {
		return nullptr;
	}
}

TShared<RemoteRoutine> RemoteComponent::Load(const String& code) {
	IScript::Request& request = script.GetDefaultRequest();
	request.DoLock();
	IScript::Request::Ref ref = request.Load(code, "RemoteComponent");
	request.UnLock();

	if (ref.value != 0) {
		return TShared<RemoteRoutine>::From(new RemoteRoutine(this, ref));
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
			// managed by remote routine
			TShared<RemoteRoutine> remoteRoutine = TShared<RemoteRoutine>::From(new RemoteRoutine(fromRequest.GetRequestPool(), ref));
			if (flag & RemoteComponent::REMOTECOMPONENT_TRANSPARENT) {
				// create wrapper
				RoutineWrapper routineWrapper(std::move(remoteRoutine));
				request << request.Adapt(WrapClosure(std::move(routineWrapper), &RoutineWrapper::operator ()));
			} else {
				request << remoteRoutine;
			}

			break;
		}
		case IScript::Request::OBJECT:
		{
			IScript::BaseDelegate d;
			fromRequest >> d;
			IScript::Object* object = d.GetRaw();
			if (flag & RemoteComponent::REMOTECOMPONENT_TRANSPARENT) {
				RemoteRoutine* routine = object->QueryInterface(UniqueType<RemoteRoutine>());
				if (routine != nullptr) {
					if (&routine->pool->GetScript() == request.GetScript()) {
						request << routine->ref;
					} else {
						TShared<RemoteRoutine> r(routine);
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

void RemoteComponent::Call(IScript::Request& fromRequest, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		toRequest.DoLock();
		fromRequest.DoLock();

		toRequest.Push();
		uint32_t flag = Flag().load(std::memory_order_relaxed);
		// read remaining parameters
		for (size_t i = 0; i < args.count; i++) {
			CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
		}
		fromRequest.UnLock();
		toRequest.Call(remoteRoutine->ref);

		fromRequest.DoLock();
		for (int k = 0; k < toRequest.GetCount(); k++) {
			CopyVariable(flag, fromRequest, toRequest, toRequest.GetCurrentType());
		}

		toRequest.Pop();
		fromRequest.UnLock();
		toRequest.UnLock();

		requestPool.ReleaseSafe(&toRequest);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

void RemoteComponent::Complete(IScript::RequestPool* returnPool, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine) {
	OPTICK_EVENT();

	toRequest.DoLock();
	toRequest.Call(remoteRoutine->ref);
	toRequest.UnLock();

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

void RemoteComponent::FinishCallAsync(IScript::Request& fromRequest, IScript::Request& toRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	toRequest.Push();
	// read remaining parameters
	uint32_t flag = Flag().load(std::memory_order_relaxed);
	for (size_t i = 0; i < args.count; i++) {
		CopyVariable(flag, toRequest, fromRequest, fromRequest.GetCurrentType());
	}

	toRequest.UnLock();
	IScript::RequestPool* pool = fromRequest.GetRequestPool();
	assert(&pool->GetScript() == fromRequest.GetScript());
	fromRequest.UnLock();

	engine.bridgeSunset.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &RemoteComponent::Complete), pool, std::ref(toRequest), callback, remoteRoutine), 1);
}

bool RemoteComponent::TryCallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		fromRequest.DoLock();
		if (toRequest.TryLock()) {
			FinishCallAsync(fromRequest, toRequest, callback, remoteRoutine, args);
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

void RemoteComponent::CallAsync(IScript::Request& fromRequest, IScript::Request::Ref callback, const TShared<RemoteRoutine>& remoteRoutine, IScript::Request::Arguments& args) {
	if (remoteRoutine->pool == this && remoteRoutine->ref) {
		IScript::Request& toRequest = *requestPool.AcquireSafe();
		fromRequest.DoLock();
		toRequest.DoLock();

		FinishCallAsync(fromRequest, toRequest, callback, remoteRoutine, args);
	} else {
		fromRequest.Error("Invalid ref.");
	}
}

