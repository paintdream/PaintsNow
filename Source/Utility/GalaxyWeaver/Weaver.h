// Weaver.h
// By PaintDream
// 2016-3-22
//

#pragma once
#include "../BridgeSunset/BridgeSunset.h"
#include "../SnowyStream/SnowyStream.h"
#include "../MythForest/MythForest.h"
#include "../../General/Misc/RemoteCall.h"
#include "Protocol.h"

namespace PaintsNow {
	class Weaver : public TReflected<Weaver, WarpTiny> {
	public:
		Weaver(BridgeSunset& bridgeSunset, SnowyStream& snowyStream, MythForest& mythForest, ITunnel& tunnel, const String& entry);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void ScriptUninitialize(IScript::Request& request) override;
		void SetRpcCallback(IScript::Request& request, const IScript::Request::Ref& ref);
		void SetConnectionCallback(IScript::Request& request, const IScript::Request::Ref& ref);

	public:
		void OnConnectionStatus(IScript::Request& request, bool state, ITunnel::EVENT status, const String& message);
		// Local controls
		void Start();
		void Stop();

	protected:
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;
		MythForest& mythForest;
		RemoteCall remoteCall;
		IScript::Request::Ref rpcCallback;
		IScript::Request::Ref connectionCallback;

	public:
		// Remote routines
		bool RpcCheckVersion(RemoteCall& remoteCall, ProtoOutputCheckVersion& outputPacket, rvalue<ProtoInputCheckVersion> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcInitialize(RemoteCall& remoteCall, ProtoOutputInitialize& outputPacket, rvalue<ProtoInputInitialize> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcUninitialize(RemoteCall& remoteCall, ProtoOutputUninitialize& outputPacket, rvalue<ProtoInputUninitialize> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);

		bool RpcDebugPrint(RemoteCall& remoteCall, ProtoOutputDebugPrint& outputPacket, rvalue<ProtoInputDebugPrint> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostResource(RemoteCall& remoteCall, ProtoOutputPostResource& outputPacket, rvalue<ProtoInputPostResource> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostEntity(RemoteCall& remoteCall, ProtoOutputPostEntity& outputPacket, rvalue<ProtoInputPostEntity> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostEntityGroup(RemoteCall& remoteCall, ProtoOutputPostEntityGroup& outputPacket, rvalue<ProtoInputPostEntityGroup> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostEntityComponent(RemoteCall& remoteCall, ProtoOutputPostEntityComponent& outputPacket, rvalue<ProtoInputPostEntityComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostModelComponent(RemoteCall& remoteCall, ProtoOutputPostModelComponent& outputPacket, rvalue<ProtoInputPostModelComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostModelComponentMaterial(RemoteCall& remoteCall, ProtoOutputPostModelComponentMaterial& outputPacket, rvalue<ProtoInputPostModelComponentMaterial> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostTransformComponent(RemoteCall& remoteCall, ProtoOutputPostTransformComponent& outputPacket, rvalue<ProtoInputPostTransformComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostSpaceComponent(RemoteCall& remoteCall, ProtoOutputPostSpaceComponent& outputPacket, rvalue<ProtoInputPostSpaceComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcPostEnvCubeComponent(RemoteCall& remoteCall, ProtoOutputPostEnvCubeComponent& outputPacket, rvalue<ProtoInputPostEnvCubeComponent> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
		bool RpcComplete(RemoteCall& remoteCall, ProtoOutputComplete& outputPacket, rvalue<ProtoInputComplete> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id);
	};
}

