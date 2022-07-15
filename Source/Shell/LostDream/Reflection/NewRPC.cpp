#include "NewRPC.h"
#include "../../../General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../../Core/Driver/Filter/Pod/ZFilterPod.h"
#include "../../../General/Misc/RemoteCall.h"

using namespace PaintsNow;

bool NewRPC::Initialize() {
	return true;
}

class InputPacket : public TReflected<InputPacket, IReflectObjectComplex> {
public:
	TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator () (reflect);

		if (reflect.IsReflectProperty()) {
			ReflectProperty(intValue);
			ReflectProperty(stringValue);
		}

		return *this;
	}

	int intValue;
	String stringValue;
};

class OutputPacket : public TReflected<OutputPacket, IReflectObjectComplex> {
public:
	TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator () (reflect);

		if (reflect.IsReflectProperty()) {
			ReflectProperty(fltValue);
			ReflectProperty(vecValue);
		}

		return *this;
	}

	float fltValue;
	std::vector<String> vecValue;
};

static bool ServerProcess(RemoteCall& remoteCall, OutputPacket& outputPacket, InputPacket& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
	if (rand() % 2 == 0) {
#if !defined(_MSC_VER) || _MSC_VER > 1200
		RemoteCall* call = &remoteCall;
		std::thread t([call, outputPacket, inputPacket, id, context]() mutable {
			MemoryStream ms(0x1000, true);
			outputPacket.fltValue = inputPacket.intValue * 10.0f;
			outputPacket.vecValue.emplace_back(inputPacket.stringValue + "_async_response");
			call->Complete(context, id, outputPacket, ms);
		});
		t.detach();
		
		return false;
#endif
	}

	outputPacket.fltValue = inputPacket.intValue / 10.0f;
	outputPacket.vecValue.emplace_back(inputPacket.stringValue + "_response");

	return true;
}

static void ClientReceive(RemoteCall& remoteCall, rvalue<OutputPacket> packetParam) {
	OutputPacket& packet = packetParam;
	printf("Receive: %f, %s\n", packet.fltValue, packet.vecValue[0].c_str());
}

static void OnClientConnect(RemoteCall& remoteCall, ITunnel::Connection* connection, RemoteCall::STATUS status) {
	// sync call
	if (status == RemoteCall::CONNECTED) {
		InputPacket inputPacket;
		inputPacket.intValue = 3389;
		inputPacket.stringValue = "hi~";
		remoteCall.Call("ServerProcess", std::move(inputPacket), Wrap(ClientReceive));
		inputPacket.stringValue = "hi2~";
		remoteCall.Call("MethodRPC.Process", std::move(inputPacket), Wrap(ClientReceive));
		remoteCall.Flush();
		printf("Server Connected ... %p\n", connection);
	} else if (status == RemoteCall::CLOSED || status == RemoteCall::ABORTED) {
		printf("Server Disconnected ... %p\n", connection);
	}
}

static void OnServerConnect(RemoteCall& remoteCall, ITunnel::Connection* connection, RemoteCall::STATUS status) {
	// sync call
	if (status == RemoteCall::CONNECTED) {
		printf("Client Connected ... %p\n", connection);
	} else if (status == RemoteCall::CLOSED || status == RemoteCall::ABORTED) {
		printf("Client Disconnected ... %p\n", connection);
	}
}

class MethodRPC : public TReflected<MethodRPC, IReflectObjectComplex> {
public:
	bool Process(RemoteCall& remoteCall, OutputPacket& outputPacket, InputPacket& inputPacket, const TShared<RemoteCall::Context>& context, uint32_t id) {
		outputPacket.fltValue = inputPacket.intValue / 10.0f;
		outputPacket.vecValue.emplace_back(inputPacket.stringValue + "_response");
		return true;
	}

	TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator ()(reflect);

		if (reflect.IsReflectMethod()) {
			ReflectMethod(Process)[RemoteMethod];
		}

		return *this;
	}
};

bool NewRPC::Run(int randomSeed, int length) {
	ZThreadPthread uniqueThreadApi;
	ZNetworkLibEvent network(uniqueThreadApi);
	ITunnel& tunnel = network;
	ZFilterPod filterPod;
	MethodRPC methodRPC;

	RemoteCall serverCall(uniqueThreadApi, tunnel, filterPod, "127.0.0.1:16384", Wrap(OnServerConnect));
	serverCall.Register("ServerProcess", Wrap(ServerProcess));
	serverCall.RegisterByObject("MethodRPC", methodRPC);
	serverCall.Start();
	uniqueThreadApi.Sleep(2000);

	for (int i = 0; i < length; i++) {
		RemoteCall clientCall(uniqueThreadApi, tunnel, filterPod);
		clientCall.Start();
		clientCall.Connect(Wrap(OnClientConnect), "127.0.0.1:16384");
		getchar();
	}

	return true;
}

void NewRPC::Summary() {}

TObject<IReflect>& NewRPC::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}