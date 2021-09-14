#include "RemoteCall.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
using namespace PaintsNow;

const size_t CHUNK_SIZE = 0x1000;

RemoteCall::Session::Session(RemoteCall& r) : remoteCall(r), connection(nullptr), inputStream(CHUNK_SIZE, true), outputStream(CHUNK_SIZE, true), requestID(0) {}

RemoteCall::Session::~Session() {
	if (connection != nullptr) {
		remoteCall.GetTunnel().CloseConnection(connection);
	}
}

void RemoteCall::Session::Flush() {
	// send pending tasks
	ITunnel& tunnel = remoteCall.GetTunnel();
	IFilterBase& filter = remoteCall.GetFilter();

	while (!requestQueue.Empty()) {
		TShared<RequestBase> request = requestQueue.Top();
		requestQueue.Pop();
		outputStream << request->name << ++requestID;
		IStreamBase* encodeStream = filter.CreateFilter(outputStream);
		request->Prepare(*encodeStream);
		encodeStream->Destroy();

		completionMap[requestID] = request;

		ITunnel::Packet packet;
		packet.header.length = verify_cast<ITunnel::PacketSizeType>(outputStream.GetOffset());
		tunnel.WriteConnectionPacket(connection, outputStream.GetBuffer(), verify_cast<ITunnel::PacketSizeType>(outputStream.GetOffset()), packet);

		outputStream.Seek(IStreamBase::BEGIN, 0);
	}
}

void RemoteCall::Session::HandleEvent(ITunnel::EVENT event) {
	ITunnel& tunnel = remoteCall.GetTunnel();
	const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& statusHandler = remoteCall.GetServerStatusHandler();
	// { CONNECTED, TIMEOUT, READ, WRITE, CLOSE, ERROR }
	switch (event) {
	case ITunnel::CONNECTED:
		if (statusHandler)
			statusHandler(remoteCall, connection, RemoteCall::CONNECTED);
		break;
	case ITunnel::TIMEOUT:
		if (statusHandler)
			statusHandler(remoteCall, connection, RemoteCall::TIMEOUT);
		break;
	case ITunnel::READ:
		// Handle for new call
		Process();
		break;
	case ITunnel::WRITE:
		break;
	case ITunnel::CLOSE:
	case ITunnel::ABORT:
		if (statusHandler) {
			if (event == ITunnel::CLOSE) {
				statusHandler(remoteCall, connection, RemoteCall::CLOSED);
			} else {
				statusHandler(remoteCall, connection, RemoteCall::ABORTED);
			}
		}

		tunnel.CloseConnection(connection);
		connection = nullptr;
		break;
	case ITunnel::AWAKE:
		Flush();
		break;
	case ITunnel::CUSTOM:
		break;
	}
}

void RemoteCall::Session::Process() {
	String segment;
	ITunnel& tunnel = remoteCall.GetTunnel();
	ITunnel::PacketSizeType blockSize = CHUNK_SIZE;
	segment.resize(blockSize);
	const std::unordered_map<String, TShared<ResponseBase> >& requestHandlers = remoteCall.GetRequestHandlers();
	IFilterBase& filter = remoteCall.GetFilter();

	while (tunnel.ReadConnectionPacket(connection, const_cast<char*>(segment.data()), blockSize, currentState)) {
		// new packet?
		size_t len = blockSize;
		inputStream.Write(segment.data(), len);
		if (currentState.cursor == currentState.header.length) {
			// full packet received
			String name;
			uint32_t id;
			inputStream >> name >> id;

			if (name.empty()) { // response received
				std::unordered_map<uint32_t, TShared<RequestBase> >::iterator it = completionMap.find(id);
				if (it != completionMap.end()) {
					(*it).second->Complete(remoteCall, inputStream);
					completionMap.erase(it);
				}
			} else {
				std::unordered_map<String, TShared<ResponseBase> >::const_iterator it = requestHandlers.find(name);
				if (it != requestHandlers.end()) {
					IStreamBase* inputEncodeStream = filter.CreateFilter(inputStream);
					IStreamBase* outputEncodeStream = filter.CreateFilter(outputStream);
					bool sync = (*it).second->Handle(remoteCall, outputStream, inputStream);
					assert(sync); // we only support synchronized call currently
					outputEncodeStream->Destroy();
					inputEncodeStream->Destroy();
				}

				ITunnel::Packet packet;
				packet.header.length = verify_cast<ITunnel::PacketSizeType>(outputStream.GetOffset());
				tunnel.WriteConnectionPacket(connection, outputStream.GetBuffer(), verify_cast<ITunnel::PacketSizeType>(outputStream.GetOffset()), packet);
				outputStream.Seek(IStreamBase::BEGIN, 0);
			}

			inputStream.Seek(IStreamBase::BEGIN, 0);
		}
	}
}

RemoteCall::RemoteCall(IThread& thread, ITunnel& t, IFilterBase& f, const String& e, const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& sh) : threadApi(thread), tunnel(t), filter(f), entry(e), serverStatusHandler(sh), dispatcher(nullptr), currentResponseID(0) {
	dispThread.store(nullptr, std::memory_order_release);
}

void RemoteCall::Stop() {
	outputSession = nullptr;
	inputSessions.clear();

	if (dispatcher != nullptr) {
		tunnel.DeactivateDispatcher(dispatcher);

		IThread::Thread* thread = dispThread.exchange(nullptr, std::memory_order_release);
		if (thread != nullptr) {
			threadApi.Wait(thread);
			threadApi.DeleteThread(thread);
		}
	}
}

RemoteCall::~RemoteCall() {
	Stop();

	if (dispatcher != nullptr) {
		tunnel.CloseDispatcher(dispatcher);
	}
}

void RemoteCall::Connect(const TWrapper<void, RemoteCall&, ITunnel::Connection*, STATUS>& clientStatusHandler, const String& target) {
	assert(dispatcher != nullptr);

	TShared<Session> session = TShared<Session>::From(new Session(*this));
	session->clientStatusHandler = clientStatusHandler;
	session->connection = tunnel.OpenConnection(dispatcher, Wrap(session(), &Session::HandleEvent), target);

	outputSession = session;
}

bool RemoteCall::ThreadProc(IThread::Thread* thread, size_t context) {
	OPTICK_THREAD("Network RemoteCall");

	ITunnel::Dispatcher* disp = dispatcher;
	ITunnel::Listener* listener = nullptr;

	if (!entry.empty()) {
		listener = tunnel.OpenListener(disp, Wrap(this, &RemoteCall::HandleEvent), Wrap(this, &RemoteCall::OnConnection), entry);
		if (listener == nullptr) {
			HandleEvent(ITunnel::ABORT);
			return false;
		}

		tunnel.ActivateListener(listener);
	}

	tunnel.ActivateDispatcher(disp); // running ...

	if (!entry.empty()) {
		tunnel.DeactivateListener(listener);
		tunnel.CloseListener(listener);
	}

	IThread::Thread* t = dispThread.exchange(nullptr, std::memory_order_release);
	if (t != nullptr) {
		assert(t == thread);
		threadApi.DeleteThread(t);
	}

	return false;
}

void RemoteCall::Flush() {
	tunnel.Wakeup(outputSession->connection);
}

void RemoteCall::HandleEvent(ITunnel::EVENT event) {
	if (event == ITunnel::AWAKE) {
		outputSession->Flush();
	}
}

void RemoteCall::Reset() {
	Stop();
	Start();
}

bool RemoteCall::Start() {
	if (dispatcher == nullptr) {
		dispatcher = tunnel.OpenDispatcher();
	}

	assert(dispThread.load(std::memory_order_relaxed) == nullptr);
	dispThread.store(threadApi.NewThread(Wrap(this, &RemoteCall::ThreadProc), 0), std::memory_order_relaxed);

	return dispThread.load(std::memory_order_relaxed) != nullptr;
}

const ITunnel::Handler RemoteCall::OnConnection(ITunnel::Connection* connection) {
	TShared<Session> session = TShared<Session>::From(new Session(*this));
	session->connection = connection;

	std::binary_insert(inputSessions, session);
	return Wrap(session(), &Session::HandleEvent);
}
