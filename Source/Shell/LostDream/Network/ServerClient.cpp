#include "ServerClient.h"
#include "../../../Core/System/ThreadPool.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../../General/Driver/Network/KCP/ZNetworkKCP.h"
using namespace PaintsNow;

bool ServerClient::Initialize() {
	return true;
}

bool ServerClient::ThreadProc(IThread::Thread* thread, size_t index) {
	INetwork& network = *networkPtr;
	INetwork::Dispatcher* dispatcher = network.OpenDispatcher();
	INetwork::Connection* connection = network.OpenConnection(dispatcher, Wrap(this, &ServerClient::ClientEventHandler), "127.0.0.1:12345");
	network.ActivateConnection(connection);
	threadApiPtr->Sleep(2000);
	// say hello~
	const char immediate[] = "hello~!";
	network.WriteConnection(connection, immediate, sizeof(immediate), ITunnel::IMMEDIATE);
	const char channel[] = "channel~!";
	network.WriteConnection(connection, channel, sizeof(channel), 0);
	const char buffered[] = "buffered~!";
	network.WriteConnection(connection, buffered, sizeof(buffered), ITunnel::BUFFERED);
	network.ActivateDispatcher(dispatcher);
	network.DeactivateConnection(connection);
	network.CloseDispatcher(dispatcher);

	return false; // do not delete our self
}

void ServerClient::ClientEventHandler(ITunnel::Connection* connection, ITunnel::EVENT event) {
	INetwork& network = *networkPtr;
	printf("RECV\n");
	if (event == ITunnel::READ) {
		char data[2048];
		while (true) {
			size_t length = 0;
			size_t mode = 0;
			if (!network.ReadConnection(connection, nullptr, length, mode))
				break;
			assert(length < sizeof(data) - 1);

			network.ReadConnection(connection, data, length, mode);
			printf("Client data read: %s with MODE = %d\n", data, (int)mode);

			if (strcmp(data, "EXIT") == 0) {
				network.DeactivateDispatcher(network.GetConnectionDispatcher(connection));
			} else if (strcmp(data, "Streammed data!") == 0) {
				const char exited[] = "EXIT";
				network.WriteConnection(connection, exited, sizeof(exited), 0);
			}
		}
	}
}

void ServerClient::ServerEventHandler(ITunnel::Connection* connection, ITunnel::EVENT event) {
	INetwork& network = *networkPtr;
	if (event == ITunnel::CONNECTED) {
		printf("SEND\n");
		const char streammed[] = "Streammed data!";
		network.WriteConnection(connection, streammed, sizeof(streammed), 0);
		const char immediate[] = "Immediate!";
		network.WriteConnection(connection, immediate, sizeof(immediate), ITunnel::IMMEDIATE);
		const char raw[] = "Raw!";
		network.WriteConnection(connection, raw, sizeof(raw), ITunnel::BUFFERED);
		const char anotherraw[] = "Another Raw!";
		network.WriteConnection(connection, anotherraw, sizeof(anotherraw), ITunnel::BUFFERED);
	} else if (event == ITunnel::READ) {
		char data[2048];
		while (true) {
			size_t length = 0;
			size_t mode = 0;
			if (!network.ReadConnection(connection, nullptr, length, mode))
				break;
			assert(length < sizeof(data) - 1);

			network.ReadConnection(connection, data, length, mode);
			printf("Server data read: %s with MODE = %d\n", data, (int)mode);

			if (strcmp(data, "EXIT") == 0) {
				const char exited[] = "EXIT";
				network.WriteConnection(connection, exited, sizeof(exited), 0);
				network.DeactivateDispatcher(network.GetConnectionDispatcher(connection));
			}
		}
	}
}

const TWrapper<void, ITunnel::Connection*, ITunnel::EVENT> ServerClient::ConnectionHandler(ITunnel::Connection* connection) {
	return Wrap(this, &ServerClient::ServerEventHandler);
}

void ServerClient::EventHandler(ITunnel::Listener* listener, ITunnel::EVENT) {}

bool ServerClient::Run(int randomSeed, int length) {
	ZThreadPthread threadApi;
	ZNetworkKCP network(threadApi);
	threadApiPtr = &threadApi;
	networkPtr = &network;

	IThread::Thread* thread = threadApi.NewThread(Wrap(this, &ServerClient::ThreadProc), 0);
	INetwork::Dispatcher* dispatcher = network.OpenDispatcher();
	INetwork::Listener* listener = network.OpenListener(dispatcher, Wrap(this, &ServerClient::EventHandler), Wrap(this, &ServerClient::ConnectionHandler), "127.0.0.1:12345");
	network.ActivateListener(listener);
	network.ActivateDispatcher(dispatcher);
	network.DeactivateListener(listener);
	network.CloseDispatcher(dispatcher);
	threadApi.Wait(thread);
	threadApi.DeleteThread(thread);

	return true;
}

void ServerClient::Summary() {}

TObject<IReflect>& ServerClient::operator ()(IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}