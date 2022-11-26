#include "Weaver.h"
#include <sstream>

using namespace PaintsNow;

static const String WEAVER_VERSION = "2018.1.25";

Weaver::Weaver(BridgeSunset& sunset, SnowyStream& ns, MythForest& mf, ITunnel& tunnel, const String& entry) : bridgeSunset(sunset), snowyStream(ns), mythForest(mf), remoteCall(sunset.GetThreadApi(), tunnel, ns.GetInterfaces().assetFilterBase, entry) {
	remoteCall.RegisterByObject("", *this);
}

void Weaver::ScriptUninitialize(IScript::Request& request) {
	IScript* script = request.GetScript();
	if (script == &bridgeSunset.GetScript()) {
		request.Dereference(rpcCallback);
		request.Dereference(connectionCallback);
	}
	
	BaseClass::ScriptUninitialize(request);
}

static void ReplaceCallback(IScript::Request& request, IScript::Request::Ref& target, const IScript::Request::Ref& ref) {
	if (target) {
		request.Dereference(target);
	}

	target = ref;
}

void Weaver::SetRpcCallback(IScript::Request& request, const IScript::Request::Ref& ref) {
	ReplaceCallback(request, rpcCallback, ref);
}

void Weaver::SetConnectionCallback(IScript::Request& request, const IScript::Request::Ref& ref) {
	ReplaceCallback(request, connectionCallback, ref);
}

void Weaver::OnConnectionStatus(IScript::Request& request, bool isAuto, ITunnel::EVENT status, const String & message) {
	if (status == ITunnel::CONNECTED || status == ITunnel::CLOSE || status == ITunnel::ABORT) {
		String code = status == ITunnel::CONNECTED ? "Connected" : status == ITunnel::CLOSE ? "Closed" : "Aborted";
		if (connectionCallback) {
			request.DoLock();
			request.Push();
			request.Call(connectionCallback, code, message);
			request.Pop();
			request.UnLock();
		}
	}

	if (status == ITunnel::CLOSE || status == ITunnel::ABORT) {
		// restart if not manually stopped
		if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
			remoteCall.Reset();
		}
	}
}

void Weaver::Start() {
	if (Flag().load(std::memory_order_acquire) & TINY_ACTIVATED) {
		Stop();
	}

	remoteCall.Start();
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
}

void Weaver::Stop() {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed);
	remoteCall.Stop();
}

TObject<IReflect>& Weaver::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RpcCheckVersion)[RemoteMethod];
		ReflectMethod(RpcComplete)[RemoteMethod];
		ReflectMethod(RpcPostResource)[RemoteMethod];
		ReflectMethod(RpcDebugPrint)[RemoteMethod];
		ReflectMethod(RpcPostEntity)[RemoteMethod];
		ReflectMethod(RpcPostEntityGroup)[RemoteMethod];
		ReflectMethod(RpcPostEntityComponent)[RemoteMethod];
		ReflectMethod(RpcPostModelComponent)[RemoteMethod];
		ReflectMethod(RpcPostModelComponentMaterial)[RemoteMethod];
		ReflectMethod(RpcPostEnvCubeComponent)[RemoteMethod];
		ReflectMethod(RpcPostTransformComponent)[RemoteMethod];
		ReflectMethod(RpcPostSpaceComponent)[RemoteMethod];
	}

	return *this;
}

bool Weaver::RpcCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion& outputPacket, rvalue<ProtoInputCheckVersion> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	// Write current version string to output buffer.
	outputPacket.clientVersion = WEAVER_VERSION;

	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcCheckVersion"), WEAVER_VERSION);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcInitialize(RemoteCall& remoteCall, ProtoOutputInitialize& outputPacket, rvalue<ProtoInputInitialize> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	return true;
}

bool Weaver::RpcUninitialize(RemoteCall& remoteCall, ProtoOutputUninitialize& outputPacket, rvalue<ProtoInputUninitialize> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	return true;
}

bool Weaver::RpcDebugPrint(RemoteCall& remoteCall, ProtoOutputDebugPrint& outputPacket, rvalue<ProtoInputDebugPrint> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputDebugPrint& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcDebugPrint"), input.text);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostResource(RemoteCall& remoteCall, ProtoOutputPostResource& outputPacket, rvalue<ProtoInputPostResource> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	ProtoInputPostResource& input = inputPacket;
	String& resourceData = input.resourceData;
	String& path = input.location;
	String& extension = input.extension;

	bool success = true;
	// Make memory stream for deserialization
	size_t length = resourceData.length();
	MemoryStream memoryStream(length);
	memoryStream.Write(resourceData.c_str(), length);
	// resource use internal persist and needn't set environment here
	// memoryStream.SetEnvironment(snowyStream);
	assert(length == resourceData.length());

	memoryStream.Seek(IStreamBase::BEGIN, 0);
	// Create resource from memory
	TShared<ResourceBase> resource = snowyStream.CreateResource(path, extension, false, ResourceBase::RESOURCE_VIRTUAL);
	resource->Map();
	IStreamBase* filter = snowyStream.GetInterfaces().assetFilterBase.CreateFilter(memoryStream);

	do {
		TSpinLockGuard<uint32_t> guard(resource->critical);
		*filter >> *resource;
	} while (false);

	if (resource) {
		// Serialize it to disk
		success = resource->Persist();
	}

	resource->UnMap();
	filter->Destroy();

	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostResource"), path, extension);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEntity(RemoteCall& remoteCall, ProtoOutputPostEntity& outputPacket, rvalue<ProtoInputPostEntity> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostEntity& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntity"), input.entityID, input.groupID, input.entityName);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEntityGroup(RemoteCall& remoteCall, ProtoOutputPostEntityGroup& outputPacket, rvalue<ProtoInputPostEntityGroup> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostEntityGroup& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntityGroup"), input.groupID, input.groupName);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEntityComponent(RemoteCall& remoteCall, ProtoOutputPostEntityComponent& outputPacket, rvalue<ProtoInputPostEntityComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostEntityComponent& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntityComponent"), input.entityID, input.componentID);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostModelComponent(RemoteCall& remoteCall, ProtoOutputPostModelComponent& outputPacket, rvalue<ProtoInputPostModelComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostModelComponent& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostModelComponent"), input.componentID, input.meshResource, input.viewDistance);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostModelComponentMaterial(RemoteCall& remoteCall, ProtoOutputPostModelComponentMaterial& outputPacket, rvalue<ProtoInputPostModelComponentMaterial> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostModelComponentMaterial& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostModelComponentMaterial"), input.componentID, input.meshGroupID, input.materialResource);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostTransformComponent(RemoteCall& remoteCall, ProtoOutputPostTransformComponent& outputPacket, rvalue<ProtoInputPostTransformComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {	
	if (rpcCallback) {
		ProtoInputPostTransformComponent& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostTransformComponent"), input.componentID, input.position, input.scale, input.rotation);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostSpaceComponent(RemoteCall& remoteCall, ProtoOutputPostSpaceComponent& outputPacket, rvalue<ProtoInputPostSpaceComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostSpaceComponent& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostSpaceComponent"), input.componentID, input.groupID);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEnvCubeComponent(RemoteCall& remoteCall, ProtoOutputPostEnvCubeComponent& outputPacket, rvalue<ProtoInputPostEnvCubeComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		ProtoInputPostEnvCubeComponent& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEnvCubeComponent"), input.componentID, input.texturePath);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcComplete(RemoteCall& remoteCall, ProtoOutputComplete& outputPacket, rvalue<ProtoInputComplete> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	if (rpcCallback) {
		// ProtoInputComplete& input = inputPacket;
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcComplete"));
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}
	
	return true;
}
