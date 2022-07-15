#include "GalaxyWeaver.h"

using namespace PaintsNow;

GalaxyWeaver::GalaxyWeaver(IThread& threadApi, ITunnel& net, BridgeSunset& sunset, SnowyStream& ns, MythForest& my) : network(net), bridgeSunset(sunset), snowyStream(ns), mythForest(my) {}

TObject<IReflect>& GalaxyWeaver::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewWeaver)[ScriptMethod = "NewWeaver"];
		ReflectMethod(RequestSetWeaverRpcCallback)[ScriptMethodLocked = "SetWeaverRpcCallback"];
		ReflectMethod(RequestSetWeaverConnectionCallback)[ScriptMethodLocked = "SetWeaverConnectionCallback"];
		ReflectMethod(RequestStartWeaver)[ScriptMethod = "StartWeaver"];
		ReflectMethod(RequestStopWeaver)[ScriptMethod = "StopWeaver"];
	}

	return *this;
}

void GalaxyWeaver::RequestSetWeaverRpcCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->SetRpcCallback(request, callback);
}

void GalaxyWeaver::RequestSetWeaverConnectionCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE_LOCKED(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->SetConnectionCallback(request, callback);
}

TShared<Weaver> GalaxyWeaver::RequestNewWeaver(IScript::Request& request, const String& config) {
	TShared<Weaver> weaver = TShared<Weaver>::From(new Weaver(bridgeSunset, snowyStream, mythForest, network, config));
	weaver->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
	return weaver;
}

void GalaxyWeaver::RequestStartWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->Start();
}

void GalaxyWeaver::RequestStopWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(weaver);
	CHECK_THREAD_IN_LIBRARY(weaver);

	weaver->Stop();
}
