#include "CrossScriptModule.h"
using namespace PaintsNow;

CrossScriptModule::CrossScriptModule(ThreadPool& t, IScript& s) : threadPool(t), script(s) {}
CrossScriptModule::~CrossScriptModule() {}

TObject<IReflect>& CrossScriptModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestLoad)[ScriptMethod = "Load"];
		ReflectMethod(RequestGet)[ScriptMethod = "Get"];
		ReflectMethod(RequestCall)[ScriptMethod = "Call"];
		ReflectMethod(RequestCallAsync)[ScriptMethod = "CallAsync"];
		ReflectMethod(RequestTryCallAsync)[ScriptMethod = "TryCallAsync"];
		ReflectMethod(RequestIsLocked)[ScriptMethodLocked = "IsLocked"];
		ReflectMethod(RequestCleanup)[ScriptMethod = "Cleanup"];
	}

	return *this;
}

TShared<CrossScript> CrossScriptModule::New(bool transparentMode, bool disableSandBoxMode) {
	TShared<CrossScript> crossScript = TShared<CrossScript>::From(new CrossScript(threadPool, *script.NewScript()));

	if (transparentMode) {
		crossScript->Flag().fetch_or(CrossScript::CROSSSCRIPT_TRANSPARENT, std::memory_order_relaxed);
	}

	if (!disableSandBoxMode) {
		IScript::Request& request = crossScript->GetScript().GetDefaultRequest();
		request.DoLock();
		request << global << key("io") << nil << endtable;
		request << global << key("os") << nil << endtable;	
		request.UnLock();
	}

	return crossScript;
}

TShared<CrossScript> CrossScriptModule::RequestNew(IScript::Request& request, bool transparentMode, bool disableSandBoxMode) {
	CHECK_REFERENCES_NONE();
	return New(transparentMode, disableSandBoxMode);
}

TShared<CrossRoutine> CrossScriptModule::RequestGet(IScript::Request& request, IScript::Delegate<CrossScript> crossScript, const String& name) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);
	// CHECK_THREAD_IN_MODULE(crossScript);

	return crossScript->Get(name);
}

TShared<CrossRoutine> CrossScriptModule::RequestLoad(IScript::Request& request, IScript::Delegate<CrossScript> crossScript, const String& code) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);

	return crossScript->Load(code);
}

void CrossScriptModule::RequestCall(IScript::Request& request, IScript::Delegate<CrossScript> crossScript, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);
	CHECK_DELEGATE(remoteRoutine);

	crossScript->Call(request, remoteRoutine.Get(), args);
}

void CrossScriptModule::RequestCallAsync(IScript::Request& request, IScript::Delegate<CrossScript> crossScript, IScript::Request::Ref callback, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);
	CHECK_DELEGATE(remoteRoutine);

	crossScript->CallAsync(request, callback, remoteRoutine.Get(), args);
}

bool CrossScriptModule::RequestTryCallAsync(IScript::Request& request, IScript::Delegate<CrossScript> crossScript, IScript::Request::Ref callback, IScript::Delegate<CrossRoutine> remoteRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);
	CHECK_DELEGATE(remoteRoutine);

	return crossScript->TryCallAsync(request, callback, remoteRoutine.Get(), args);
}

bool CrossScriptModule::RequestIsLocked(IScript::Request& request, IScript::Delegate<CrossScript> crossScript) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);

	return crossScript->IsLocked();
}

void CrossScriptModule::RequestCleanup(IScript::Request& request, IScript::Delegate<CrossScript> crossScript) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(crossScript);

	crossScript->requestPool.Clear();
}