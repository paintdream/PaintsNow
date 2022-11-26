#include "Coordinator.h"
using namespace PaintsNow;

Coordinator::Coordinator(IThread& threadApi, ITunnel& tunnel, IFilterBase& filter, const String& entry) : RemoteCall(threadApi, tunnel, filter, entry) {}

Coordinator::Session::Session(RemoteCall& remoteCall) : BaseClass(remoteCall) {}

const String& Coordinator::Session::GetListenAddress() const {
	return listenAddress;
}

void Coordinator::Session::SetListenAddress(const String& address) {
	listenAddress = address;
}

void Coordinator::Session::HandleEvent(ITunnel::Connection* connection, ITunnel::EVENT status) {
	BaseClass::HandleEvent(connection, status);
	// TODO: other process
}

void Coordinator::PushRequest(rvalue<RequestInfo> requestInfo) {
	pendingRequestInfos.Push(std::move(requestInfo));
}

bool Coordinator::PullRequest(RequestInfo& requestInfo) {
	if (!pendingRequestInfos.Empty()) {
		requestInfo = std::move(pendingRequestInfos.Top());
		pendingRequestInfos.Pop();
		return true;
	} else {
		return false;
	}
}

void Coordinator::CompleteRequest(rvalue<RequestInfo> requestInfo) {
	completedRequestInfos.Push(std::move(requestInfo));
}

void Coordinator::AddUpstream(const String& listenAddress) {
	BinaryInsert(upstreams, listenAddress);
}

void Coordinator::RemoveUpstream(const String& listenAddress) {
	BinaryErase(upstreams, listenAddress);
}

void Coordinator::AddDownstream(const String& listenAddress) {
	BinaryInsert(downstreams, listenAddress);
}

void Coordinator::RemoveDownstream(const String& listenAddress) {
	BinaryErase(downstreams, listenAddress);
}

void Coordinator::AddActiveSession(const TShared<Session>& session) {
	activeSessions[session->GetListenAddress()] = session;
}

void Coordinator::RemoveActiveSession(const TShared<Session>& session) {
	activeSessions.erase(session->GetListenAddress());
}

void Coordinator::RemoveSession(const TShared<RemoteCall::Session>& session) {
	RemoveActiveSession(static_cast<Session*>(session()));
	RemoteCall::RemoveSession(session);
}

TShared<RemoteCall::Session> Coordinator::CreateSession() {
	return TShared<RemoteCall::Session>::From(new Session(*this));
}

// Remote Call handlers
TObject<IReflect>& Coordinator::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RpcLogin)[RemoteMethod];
		ReflectMethod(RpcLogout)[RemoteMethod];
		ReflectMethod(RpcPush)[RemoteMethod];
		ReflectMethod(RpcPull)[RemoteMethod];
		ReflectMethod(RpcVerbose)[RemoteMethod];
		ReflectMethod(RpcComplete)[RemoteMethod];
	}

	return *this;
}

bool Coordinator::RpcLogin(RemoteCall& remoteCall, ProtoOutputLogin& outputPacket, rvalue<ProtoInputLogin> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	ProtoInputLogin& input = inputPacket;
	Session* session = static_cast<Session*>(context());
	session->SetListenAddress(input.listenAddress);
	AddActiveSession(session);

	return true;
}

bool Coordinator::RpcLogout(RemoteCall& remoteCall, ProtoOutputLogout& outputPacket, rvalue<ProtoInputLogout> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	ProtoInputLogout& input = inputPacket;
	Session* session = static_cast<Session*>(context());
	RemoveActiveSession(session);
	session->SetListenAddress("");

	return true;
}

bool Coordinator::RpcPush(RemoteCall& remoteCall, ProtoOutputPush& outputPacket, rvalue<ProtoInputPush> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	// by now we just support all protocols
	ProtoInputPull request;
	ProtoInputPush& input = inputPacket;
	request.cookie = input.cookie;
	context->Invoke("RpcPull", request, Wrap(this, &Coordinator::RetPull));
	return true;
}

bool Coordinator::RpcPull(RemoteCall& remoteCall, ProtoOutputPull& outputPacket, rvalue<ProtoInputPull> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	return false;
}

bool Coordinator::RpcVerbose(RemoteCall& remoteCall, ProtoOutputVerbose& outputPacket, rvalue<ProtoInputVerbose> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	return false;
}

bool Coordinator::RpcComplete(RemoteCall& remoteCall, ProtoOutputComplete& outputPacket, rvalue<ProtoInputComplete> inputPacket, const TShared<RemoteCall::Session>& context, uint32_t id) {
	return false;
}

// return backs
void Coordinator::RetPull(RemoteCall& remoteCall, rvalue<ProtoOutputPull> outputPacket) {
	// queryRequestMap.find(outputPacket);
}

