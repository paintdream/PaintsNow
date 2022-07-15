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

void Weaver::OnConnectionStatus(IScript::Request& request, bool isAuto, RemoteCall::STATUS status, const String & message) {
	if (status == RemoteCall::CONNECTED || status == RemoteCall::CLOSED || status == RemoteCall::ABORTED) {
		String code = status == RemoteCall::CONNECTED ? "Connected" : status == RemoteCall::CLOSED ? "Closed" : "Aborted";
		if (connectionCallback) {
			request.DoLock();
			request.Push();
			request.Call(connectionCallback, code, message);
			request.Pop();
			request.UnLock();
		}
	}

	if (status == RemoteCall::CLOSED || status == RemoteCall::ABORTED) {
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

bool Weaver::RpcCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion& outputPacket, ProtoInputCheckVersion& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
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

bool Weaver::RpcInitialize(RemoteCall& remoteCall, ProtoOutputInitialize& outputPacket, ProtoInputInitialize& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	return true;
}

bool Weaver::RpcUninitialize(RemoteCall& remoteCall, ProtoOutputUninitialize& outputPacket, ProtoInputUninitialize& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	return true;
}

bool Weaver::RpcDebugPrint(RemoteCall& remoteCall, ProtoOutputDebugPrint& outputPacket, ProtoInputDebugPrint& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcDebugPrint"), inputPacket.text);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostResource(RemoteCall& remoteCall, ProtoOutputPostResource& outputPacket, ProtoInputPostResource& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	String& resourceData = inputPacket.resourceData;
	String& path = inputPacket.location;
	String& extension = inputPacket.extension;

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

bool Weaver::RpcPostEntity(RemoteCall& remoteCall, ProtoOutputPostEntity& outputPacket, ProtoInputPostEntity& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntity"), inputPacket.entityID, inputPacket.groupID, inputPacket.entityName);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEntityGroup(RemoteCall& remoteCall, ProtoOutputPostEntityGroup& outputPacket, ProtoInputPostEntityGroup& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntityGroup"), inputPacket.groupID, inputPacket.groupName);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEntityComponent(RemoteCall& remoteCall, ProtoOutputPostEntityComponent& outputPacket, ProtoInputPostEntityComponent& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEntityComponent"), inputPacket.entityID, inputPacket.componentID);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostModelComponent(RemoteCall& remoteCall, ProtoOutputPostModelComponent& outputPacket, ProtoInputPostModelComponent& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostModelComponent"), inputPacket.componentID, inputPacket.meshResource, inputPacket.viewDistance);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostModelComponentMaterial(RemoteCall& remoteCall, ProtoOutputPostModelComponentMaterial& outputPacket, ProtoInputPostModelComponentMaterial& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostModelComponentMaterial"), inputPacket.componentID, inputPacket.meshGroupID, inputPacket.materialResource);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostTransformComponent(RemoteCall& remoteCall, ProtoOutputPostTransformComponent& outputPacket, ProtoInputPostTransformComponent& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {	
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostTransformComponent"), inputPacket.componentID, inputPacket.position, inputPacket.scale, inputPacket.rotation);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostSpaceComponent(RemoteCall& remoteCall, ProtoOutputPostSpaceComponent& outputPacket, ProtoInputPostSpaceComponent& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostSpaceComponent"), inputPacket.componentID, inputPacket.groupID);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcPostEnvCubeComponent(RemoteCall& remoteCall, ProtoOutputPostEnvCubeComponent& outputPacket, ProtoInputPostEnvCubeComponent& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
		IScript::Request& request = *bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Push();
		request.Call(rpcCallback, String("RpcPostEnvCubeComponent"), inputPacket.componentID, inputPacket.texturePath);
		request.Pop();
		request.UnLock();
		bridgeSunset.requestPool.ReleaseSafe(&request);
	}

	return true;
}

bool Weaver::RpcComplete(RemoteCall& remoteCall, ProtoOutputComplete& outputPacket, ProtoInputComplete& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rpcCallback) {
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
