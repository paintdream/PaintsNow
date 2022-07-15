#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef _EVENT_HAVE_PTHREADS
#define _EVENT_HAVE_PTHREADS
#endif

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif

#define _WIN32_WINNT 0x0500
#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#endif

#include "ZNetworkKCP.h"
#include "../../../../Core/Template/TAtomic.h"
#include "../../../../Core/Template/TQueue.h"
#include "../../../../Core/System/Tiny.h"
#include "../../../../General/Interface/ITimer.h"
#include <algorithm>
#include <sstream>

#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#define closesocket close
typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
#endif

#include "Core/ikcp.h"

using namespace PaintsNow;

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif // _MSC_VER

static const uint32_t KCP_CONV = 0x0CAD0716;
static const uint32_t RAW_CONV = 0x804D5000;
static const uint32_t BUFFER_SIZE = 1536;
static const uint32_t MAX_PACKET_SIZE = 1400;

struct ListenerKCPImpl;
struct ConnectionKCPImpl;
struct KCPBase;

static String Ntoa(const sockaddr_in& addr) {
	char sz[64];
	const char* v = reinterpret_cast<const char*>(&addr.sin_addr);
	sprintf(sz, "%d.%d.%d.%d", (int)v[0], (int)v[1], (int)v[2], (int)v[3]);

	return sz;
}

static sockaddr_in ResolveHost(const String& host) {
	String hostAddress = host;
	const char* port = strrchr(hostAddress.c_str(), ':');
	sockaddr_in sin = { 0 };
	sin.sin_family = AF_INET;
	sin.sin_port = ::htons(port == nullptr ? 10716 : atoi(port + 1));
	if (port != nullptr) {
		hostAddress[port - hostAddress.c_str()] = '\0';
	}

	*reinterpret_cast<uint32_t*>(&sin.sin_addr) = ::inet_addr(hostAddress.c_str());

	return sin;
}

struct DispatcherKCPImpl : public INetwork::Dispatcher {
	DispatcherKCPImpl(ZNetworkKCP* kcp) : api(kcp), instance(nullptr), timeOut(25), interval(50), liveTime(5000), nodelay(1), resend(1) {}

	ZNetworkKCP* api;
	KCPBase* instance;
	uint32_t liveTime;
	uint32_t timeOut;
	int interval;
	int nodelay;
	int resend;
};

struct KCPBase : public TReflected<KCPBase, SharedTiny> {
	KCPBase() : dispatcher(nullptr), socket(0), kcp(nullptr) {}
	~KCPBase() override {
		ikcp_release(kcp);

		if (socket != 0) {
			::closesocket(socket);
		}
	}

	virtual void ReceiveData(uint64_t timestamp, const sockaddr_in& addr, const char* data, int length) = 0;
	virtual bool FlushData(uint64_t timestamp, uint32_t limit) = 0;

	DispatcherKCPImpl* dispatcher;
	String host;
	SOCKET socket;
	ikcpcb* kcp;
};

typedef TQueueList<char> Buffer;
typedef TQueueFrame<Buffer> Frame;

struct ListenerKCPImpl;
struct ConnectionKCPImpl : public KCPBase, public INetwork::Connection {
	ConnectionKCPImpl() : frameDataRead(pendingRawDataRead), frameDataWrite(pendingRawDataWrite), timestamp(0), hostListener(nullptr) {
		totalReadLength.store(0, std::memory_order_relaxed);
		totalWriteLength.store(0, std::memory_order_relaxed);
	}

	~ConnectionKCPImpl() override {
		if (hostListener != nullptr) {
			socket = 0;
		}
	}

	bool ReadData(char* data, size_t& length, size_t& mode) {
		if (frameDataRead.Count() != 0 || frameDataRead.Acquire()) {
			mode = ITunnel::IMMEDIATE | ITunnel::BUFFERED;

			if (data == nullptr) {
				length = frameDataRead.Count();
			} else {
				frameDataRead.Pop(data, data + length);
			}

			return true;
		} else {
			mode = 0;

			if (data == nullptr) {
				int size = ikcp_peeksize(kcp);
				if (size >= 0) {
					length = size;
					return true;
				} else {
					return false;
				}
			} else {
				return ikcp_recv(kcp, data, verify_cast<int>(length)) >= 0;
			}
		}
	}

	bool WriteData(const char* data, int length, size_t mode) {
		if (mode & ITunnel::BUFFERED) {
			frameDataWrite.Push(data, data + length);
			frameDataWrite.Release();
			totalWriteLength.fetch_add(length, std::memory_order_release);
			return true;
		} else if (mode & ITunnel::IMMEDIATE) {
			assert(*reinterpret_cast<const uint32_t*>(data) != KCP_CONV);
			if (*reinterpret_cast<const uint32_t*>(data) != KCP_CONV) {
				return ::sendto(socket, data, verify_cast<int>(length), 0, (const sockaddr*)&target, sizeof(sockaddr_in)) != -1;
			} else {
				return false;
			}
		} else {
			ikcp_send(kcp, reinterpret_cast<const char*>(data), length);
			totalWriteLength.fetch_add(length, std::memory_order_release);
			return true;
		}
	}

	void ReceiveData(uint64_t ts, const sockaddr_in& addr, const char* data, int length) override final {
		if (length < sizeof(uint32_t))
			return;

		uint32_t sig = *reinterpret_cast<const uint32_t*>(data);
		if (sig == KCP_CONV) {
			ikcp_input(kcp, data, length);
		} else {
			frameDataRead.Push(data, data + length);
			frameDataRead.Release();
		}

		totalReadLength.fetch_add(length, std::memory_order_release);
		timestamp = ts;
	}

	bool FlushData(uint64_t ts, uint32_t limit) override final {
		if (ikcp_check(kcp, (int32_t)ts)) {
			ikcp_update(kcp, (int32_t)ts);
		}

		if (ts - timestamp < limit) {
			if (totalReadLength.exchange(0, std::memory_order_relaxed) != 0) {
				eventHandler(this, ITunnel::READ);
			}

			if (totalWriteLength.exchange(0, std::memory_order_relaxed) != 0) {
				size_t buffer[BUFFER_SIZE / sizeof(size_t)];
				while (frameDataWrite.Acquire()) {
					if (frameDataWrite.Count() < BUFFER_SIZE) {
						int length = verify_cast<int>(frameDataWrite.Count());
						frameDataWrite.Pop(reinterpret_cast<char*>(buffer), reinterpret_cast<char*>(buffer) + length);
						buffer[length] = 0;
						::sendto(socket, reinterpret_cast<char*>(buffer), length, 0, (const sockaddr*)&target, sizeof(sockaddr_in));
					}
				}
			
				ikcp_flush(kcp);
			}

			return true;
		} else {
			return false;
		}
	}

	void Connect() {
		eventHandler(this, ITunnel::CONNECTED);
	}

	void Disconnect() {
		eventHandler(this, ITunnel::CLOSE);
	}

	ListenerKCPImpl* hostListener;
	TWrapper<void, ITunnel::Connection*, ITunnel::EVENT> eventHandler;
	sockaddr_in target;
	Buffer pendingRawDataRead;
	Buffer pendingRawDataWrite;
	Frame frameDataRead;
	Frame frameDataWrite;
	uint64_t timestamp;
	std::atomic<uint32_t> totalReadLength;
	std::atomic<uint32_t> totalWriteLength;
};

static uint64_t EncodeSockAddr(const sockaddr_in& addr) {
	return (((uint64_t)addr.sin_port << 31) << 1) | (uint64_t)*reinterpret_cast<const uint32_t*>(&addr.sin_addr);
}

#if defined(_MSC_VER) && _MSC_VER <= 1200
template <>
struct hash<uint64_t> {
	size_t operator () (uint64_t v) const {
		return (size_t)v;
	}
};
#endif

template <class T, class D>
static T* CreateImpl(UniqueType<T>, DispatcherKCPImpl* dispatcher, const String& host, const D& eventHandler) {
	sockaddr_in sin = ResolveHost(host);
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) {
		return nullptr;
	}

	if (::bind(s, reinterpret_cast<const sockaddr*>(&sin), sizeof(sin)) == SOCKET_ERROR) {
		::closesocket(s);
		return nullptr;
	}

	T* impl = new T();
	impl->dispatcher = dispatcher;
	impl->socket = s;
	impl->eventHandler = eventHandler;
	impl->kcp = ikcp_create(KCP_CONV, impl);
	ikcp_setoutput(impl->kcp, &ZNetworkKCP::KCPOutput);

	return impl;
}

struct ListenerKCPImpl : public KCPBase, public INetwork::Listener {
	void ReceiveData(uint64_t ts, const sockaddr_in& addr, const char* data, int length) override final {
		uint64_t key = EncodeSockAddr(addr);
		std::unordered_map<uint64_t, TShared<ConnectionKCPImpl> >::iterator it = targets.find(key);
		ConnectionKCPImpl* impl;
		if (it == targets.end()) {
			impl = new ConnectionKCPImpl();
			impl->dispatcher = dispatcher;
			impl->socket = socket;
			impl->eventHandler = callbackHandler(impl);
			impl->kcp = ikcp_create(KCP_CONV, impl);
			ikcp_setoutput(impl->kcp, &ZNetworkKCP::KCPOutput);
			impl->host = Ntoa(addr);
			impl->hostListener = this;
			impl->target = addr;

			targets[key] = TShared<ConnectionKCPImpl>::From(impl);
			impl->Connect();
		} else {
			impl = (*it).second();
		}

		impl->ReceiveData(ts, addr, data, length);
	}

	bool FlushData(uint64_t ts, uint32_t limit) override final {
		std::unordered_map<uint64_t, TShared<ConnectionKCPImpl> >::iterator it = targets.begin();
		while (it != targets.end()) {
			ConnectionKCPImpl* impl = (*it).second();
			if (!impl->FlushData(ts, limit)) {
				impl->Disconnect();
				targets.erase(it++);
			} else {
				++it;
			}
		}

		return false;
	}

	TWrapper<void, ITunnel::Listener*, ITunnel::EVENT> eventHandler;
	TWrapper<const TWrapper<void, ITunnel::Connection*, ITunnel::EVENT>, ITunnel::Connection*> callbackHandler;
	std::unordered_map<uint64_t, TShared<ConnectionKCPImpl> > targets;
};

template <class T>
static void DestroyImpl(T* impl) {
	impl->ReleaseObject();
}

ZNetworkKCP::ZNetworkKCP(IThread& t, uint32_t mtu) : threadApi(t), maxPacketSize(Math::Min(MAX_PACKET_SIZE, mtu)) {}
ZNetworkKCP::~ZNetworkKCP() {}

ITunnel::Dispatcher* ZNetworkKCP::OpenDispatcher() {
	return new DispatcherKCPImpl(this);
}

bool ZNetworkKCP::ActivateDispatcher(Dispatcher* dispatcher) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	if (disp->instance == nullptr)
		return false;

	size_t buffer[BUFFER_SIZE / sizeof(size_t)];
#ifdef _WIN32
	::setsockopt(disp->instance->socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&disp->timeOut), sizeof(disp->timeOut));
#else
	struct timeval timeOut = { 0, (long)verify_cast<long>(disp->timeOut) * 1000 };
	::setsockopt(disp->instance->socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeOut), sizeof(timeOut));
#endif
	int64_t timer = ITimer::GetSystemClock();
	ikcp_nodelay(disp->instance->kcp, disp->nodelay, disp->interval, disp->resend, 1);

	KCPBase* instance = disp->instance;
	while (true) {
		sockaddr_in from = { 0 };
		socklen_t addrlen = sizeof(sockaddr_in);
		int length = ::recvfrom(instance->socket, reinterpret_cast<char*>(buffer), BUFFER_SIZE, 0, (sockaddr*)&from, &addrlen);
		int64_t now = ITimer::GetSystemClock();
		if (length >= 0) {
			instance->ReceiveData(now, from, reinterpret_cast<char*>(buffer), length);
		}

		if (now > timer + disp->interval) {
			instance->FlushData(now, disp->liveTime);
		}

		std::atomic_thread_fence(std::memory_order_acquire);
		if (disp->instance == nullptr) {
			// flush all data by force
			instance->FlushData(timer + disp->interval * 64, disp->liveTime);
			break;
		}
	}

	return true;
}

void ZNetworkKCP::SetDispatcherOption(Dispatcher* dispatcher, const String& option, size_t value) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	if (option == "TimeOut") {
		disp->timeOut = verify_cast<uint32_t>(value);
	} else if (option == "LiveTime") {
		disp->liveTime = verify_cast<uint32_t>(value);
	} else if (option == "Interval") {
		disp->interval = verify_cast<int>(value);
	} else if (option == "NoDelay") {
		disp->nodelay = verify_cast<int>(value);
	} else if (option == "Resend") {
		disp->resend = verify_cast<int>(value);
	}
}

void ZNetworkKCP::DeactivateDispatcher(Dispatcher* dispatcher) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	disp->instance = nullptr;
	std::atomic_thread_fence(std::memory_order_release);
}

std::vector<String> ZNetworkKCP::Resolve(const String& name) {
	addrinfo* info;
	addrinfo hints;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	std::vector<String> addresses;
	if (::getaddrinfo(name.c_str(), nullptr, &hints, &info) != 0)
		return addresses;

	for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
		addresses.emplace_back(Ntoa(*reinterpret_cast<const sockaddr_in*>(p->ai_addr)));
	}

	::freeaddrinfo(info);
	return addresses;
}

INetwork::Listener* ZNetworkKCP::OpenListener(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Listener*, EVENT>& eventHandler, const TWrapper<const TWrapper<void, ITunnel::Connection*, EVENT>, Connection*>& callbackHandler, const String& host) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	if (disp->instance != nullptr) // already bound
		return nullptr;

	ListenerKCPImpl* impl = CreateImpl(UniqueType<ListenerKCPImpl>(), disp, host, eventHandler);
	impl->callbackHandler = callbackHandler;
	disp->instance = impl;

	return impl;
}

int ZNetworkKCP::KCPOutput(const char* buf, int len, ikcpcb* kcp, void* user) {
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(user);
	return ::sendto(impl->socket, buf, len, 0, (const sockaddr*)&impl->target, sizeof(sockaddr_in));
}

bool ZNetworkKCP::ActivateListener(Listener* listener) {
	return true;
}

void ZNetworkKCP::DeactivateListener(Listener* l) {}

String ZNetworkKCP::GetAddress(Listener* listener) {
	ListenerKCPImpl* impl = static_cast<ListenerKCPImpl*>(listener);
	return impl->host;
}

void ZNetworkKCP::CloseListener(Listener* listener) {
	ListenerKCPImpl* impl = static_cast<ListenerKCPImpl*>(listener);
	DestroyImpl(impl);
}

INetwork::Connection* ZNetworkKCP::OpenConnection(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Connection*, EVENT>& eventHandler, const String& host) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	if (disp->instance != nullptr) // already bound
		return nullptr;

	ConnectionKCPImpl* impl = CreateImpl(UniqueType<ConnectionKCPImpl>(), disp, "0.0.0.0:0", eventHandler);
	impl->target = ResolveHost(host);
	disp->instance = impl;

	return impl;
}

bool ZNetworkKCP::ActivateConnection(Connection* c) {
	return true;
}

void ZNetworkKCP::DeactivateConnection(Connection* c) {}

void ZNetworkKCP::Flush(Connection* c) {
	assert(c != nullptr);
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(c);
	ikcp_flush(impl->kcp);
}

bool ZNetworkKCP::WriteConnection(Connection* c, const void* data, size_t length, size_t mode) {
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(c);
	return impl->WriteData(reinterpret_cast<const char*>(data), verify_cast<int>(length), mode);
}

bool ZNetworkKCP::ReadConnection(Connection* connection, void* data, size_t& length, size_t& mode) {
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(connection);
	return impl->ReadData(reinterpret_cast<char*>(data), length, mode);
}

void ZNetworkKCP::GetConnectionAddresses(INetwork::Connection* connection, String& src, String& dst) {
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(connection);
	dst = impl->host;
	src = "0.0.0.0:0";
}

void ZNetworkKCP::CloseConnection(Connection* c) {
	DestroyImpl(static_cast<ConnectionKCPImpl*>(c));
}

void ZNetworkKCP::SetListenerForHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& httpRequestHandler) {
	assert(false);
}

void ZNetworkKCP::PrepareHttpRequest(HttpRequest* req) {
	assert(false);
}

INetwork::HttpRequest* ZNetworkKCP::OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback) {
	assert(false);
	return nullptr;
}

void ZNetworkKCP::CloseHttpRequest(INetwork::HttpRequest* request) {
	assert(false);
}

String ZNetworkKCP::GetHttpRequestUri(HttpRequest* request) {
	assert(false);
	return "";
}

void ZNetworkKCP::SetHttpRequestUri(HttpRequest* request, const String& uri) {
	assert(false);
}

String ZNetworkKCP::GetHttpRequestMethod(HttpRequest* request) {
	assert(false);
	return "";
}

void ZNetworkKCP::SetHttpRequestMethod(HttpRequest* request, const String& method) {
	assert(false);
}

void ZNetworkKCP::GetHttpRequestHeader(HttpRequest* request, std::list<std::pair<String, String> >& header) {
	assert(false);
}

void ZNetworkKCP::SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header) {
	assert(false);
}

void ZNetworkKCP::ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment) {
	assert(false);
}

String ZNetworkKCP::MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment) {
	assert(false);
	return "";
}

void ZNetworkKCP::MakeHttpRequest(HttpRequest* request) {
	assert(false);
}

void ZNetworkKCP::MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data) {
	assert(false);
}

String ZNetworkKCP::GetHttpRequestData(HttpRequest* request) {
	assert(false);
	return "";
}

void ZNetworkKCP::SetHttpRequestData(HttpRequest* request, const String& data) {
	assert(false);
}

ITunnel::Dispatcher* ZNetworkKCP::GetListenerDispatcher(Listener* listener) {
	ListenerKCPImpl* impl = static_cast<ListenerKCPImpl*>(listener);
	return impl->dispatcher;
}

ITunnel::Dispatcher* ZNetworkKCP::GetConnectionDispatcher(Connection* connection) {
	ConnectionKCPImpl* impl = static_cast<ConnectionKCPImpl*>(connection);
	return impl->dispatcher;
}

void ZNetworkKCP::CloseDispatcher(Dispatcher* dispatcher) {
	DispatcherKCPImpl* disp = static_cast<DispatcherKCPImpl*>(dispatcher);
	delete disp;
}
