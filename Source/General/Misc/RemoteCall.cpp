#include "RemoteCall.h"
using namespace PaintsNow;

const size_t CHUNK_SIZE = 0x1000;

namespace PaintsNow {
	MetaRemoteMethod RemoteMethod;
}

RemoteCall::Session::Session(RemoteCall& r) : remoteCall(r), connection(nullptr), inputStream(CHUNK_SIZE, true), outputStream(CHUNK_SIZE, true), requestID(0) {
}

RemoteCall::Session::~Session() {
	if (connection != nullptr) {
		remoteCall.GetTunnel().CloseConnection(connection);
	}
}

void RemoteCall::Session::Flush() {
	// send pending tasks
	ITunnel& tunnel = remoteCall.GetTunnel();
	IFilterBase& filter = remoteCall.GetFilter();
	IThread& threadApi = remoteCall.GetThreadApi();

	while (!requestQueue.Empty()) {
		TShared<RequestBase> request = requestQueue.Top();
		requestQueue.Pop();
		outputStream << request->name << ++requestID;
		IStreamBase* encodeStream = filter.CreateFilter(outputStream);
		request->Prepare(*encodeStream);
		encodeStream->Destroy();

		completionMap[requestID] = request;
		tunnel.WriteConnection(connection, outputStream.GetBuffer(), outputStream.GetOffset(), 0);
		outputStream.Seek(IStreamBase::BEGIN, 0);
	}

	tunnel.Flush(connection);
}

ITunnel::Connection* RemoteCall::Session::GetConnection() const {
	return connection;
}

void RemoteCall::Session::HandleEvent(ITunnel::Connection* connection, ITunnel::EVENT event) {
	// { CONNECTED, TIMEOUT, READ, WRITE, CLOSE, ERROR }
	switch (event) {
		case ITunnel::CONNECTED:
			break;
		case ITunnel::TIMEOUT:
			break;
		case ITunnel::READ:
			// Handle for new call
			Process();
			Flush();
			break;
		case ITunnel::WRITE:
			break;
		case ITunnel::CLOSE:
		case ITunnel::ABORT:
			remoteCall.GetTunnel().CloseConnection(connection);
			this->connection = nullptr;
			remoteCall.RemoveSession(this);
			break;
		case ITunnel::AWAKE:
			Flush();
			break;
		case ITunnel::CUSTOM:
			break;
	}
}

bool RemoteCall::Session::IsPassive() const {
	return !!(Flag().load(std::memory_order_relaxed) & SESSION_PASSIVE_CONNECTION);
}

void RemoteCall::Session::Process() {
	ITunnel& tunnel = remoteCall.GetTunnel();
	const std::unordered_map<String, TShared<ResponseBase> >& requestHandlers = remoteCall.GetRequestHandlers();
	IFilterBase& filter = remoteCall.GetFilter();
	IThread& threadApi = remoteCall.GetThreadApi();

	while (true) {
		size_t packetSize = 0;
		size_t mode = 0;
		if (!tunnel.ReadConnection(connection, nullptr, packetSize, mode))
			break;

		assert(inputStream.GetOffset() == 0);
		if (inputStream.GetTotalLength() < packetSize) {
			inputStream.Extend(packetSize);
		}

		mode = 0;
		if (tunnel.ReadConnection(connection, reinterpret_cast<char*>(inputStream.GetBuffer()), packetSize, mode)) {
			// full packet received
			String name;
			uint32_t id;
			inputStream >> name >> id;

			if (name.empty()) { // response received
				std::unordered_map<uint32_t, TShared<RequestBase> >::iterator it = completionMap.find(id);
				if (it != completionMap.end()) {
					IStreamBase* inputEncodeStream = filter.CreateFilter(inputStream);
					(*it).second->Complete(remoteCall, *inputEncodeStream);
					inputEncodeStream->Destroy();
					completionMap.erase(it);
				}
			} else {
				std::unordered_map<String, TShared<ResponseBase> >::const_iterator it = requestHandlers.find(name);
				if (it != requestHandlers.end()) {
					IStreamBase* inputEncodeStream = filter.CreateFilter(inputStream);
					IStreamBase* outputEncodeStream = filter.CreateFilter(outputStream);

					outputStream << String("") << id;
					bool sync = (*it).second->Handle(remoteCall, *outputEncodeStream, *inputEncodeStream, this, id);
					outputEncodeStream->Destroy();
					inputEncodeStream->Destroy();

					if (sync) {
						tunnel.WriteConnection(connection, outputStream.GetBuffer(), outputStream.GetOffset(), 0);
					}
					
					outputStream.Seek(IStreamBase::BEGIN, 0);
				}
			}

			inputStream.Seek(IStreamBase::BEGIN, 0);
		} else {
			break;
		}
	}

	tunnel.Flush(connection);
}

RemoteCall::RemoteCall(IThread& thread, ITunnel& t, IFilterBase& f, const String& e) : threadApi(thread), tunnel(t), filter(f), entry(e), dispatcher(nullptr), currentResponseID(0) {
	dispThread.store(nullptr, std::memory_order_release);
}

void RemoteCall::Stop() {
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

TShared<RemoteCall::Session> RemoteCall::Connect(const String& target) {
	assert(dispatcher != nullptr);

	TShared<Session> session = CreateSession();
	session->connection = tunnel.OpenConnection(dispatcher, Wrap(session(), &Session::HandleEvent), target);
	tunnel.ActivateConnection(session->connection);

	return session;
}

bool RemoteCall::ThreadProc(IThread::Thread* thread, size_t context) {
	ITunnel::Dispatcher* disp = dispatcher;
	ITunnel::Listener* listener = nullptr;

	if (!entry.empty()) {
		listener = tunnel.OpenListener(disp, Wrap(this, &RemoteCall::HandleEvent), Wrap(this, &RemoteCall::OnConnection), entry);
		if (listener == nullptr) {
			HandleEvent(listener, ITunnel::ABORT);
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

// empty implementation by default
void RemoteCall::HandleEvent(ITunnel::Listener* listener, ITunnel::EVENT event) {}

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

void RemoteCall::RegisterByResponse(const String& name, const TShared<ResponseBase>& response) {
	requestHandlers[name] = response;
}

const TWrapper<void, ITunnel::Connection*, ITunnel::EVENT> RemoteCall::OnConnection(ITunnel::Connection* connection) {
	TShared<Session> session = CreateSession();
	session->connection = connection;
	session->Flag().fetch_or(Session::SESSION_PASSIVE_CONNECTION, std::memory_order_relaxed);
	AddSession(session);

	return Wrap(session(), &Session::HandleEvent);
}

TShared<RemoteCall::Session> RemoteCall::CreateSession() {
	return TShared<RemoteCall::Session>::From(new Session(*this));
}

void RemoteCall::AddSession(const TShared<Session>& session) {
	BinaryInsert(sessionHolders, session);
}

void RemoteCall::RemoveSession(const TShared<Session>& session) {
	BinaryErase(sessionHolders, session);
}

class RemoteCallRegistar : public IReflect {
public:
	RemoteCallRegistar(RemoteCall& call, const String& p) : IReflect(false, true, false, false), remoteCall(call), prefix(p) {}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {
		while (meta != nullptr) {
			const MetaRemoteMethod::TypedBase* remote = meta->GetNode()->QueryInterface(UniqueType<MetaRemoteMethod::TypedBase>());
			if (remote != nullptr) {
				String n = remote->key.size() == 0 ? name : remote->key;
				remoteCall.RegisterByResponse(prefix.size() == 0 ? n : prefix + "." + n, remote->response);
				break;
			}

			meta = meta->GetNext();
		}
	}

private:
	RemoteCall& remoteCall;
	String prefix;
};

void RemoteCall::RegisterByObject(const String& prefix, IReflectObject& object) {
	RemoteCallRegistar registar(*this, prefix);
	object(registar);
}

