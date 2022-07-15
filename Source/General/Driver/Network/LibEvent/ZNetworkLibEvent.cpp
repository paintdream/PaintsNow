#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef _EVENT_HAVE_PTHREADS
#define _EVENT_HAVE_PTHREADS
#endif

#ifdef _WIN32
#ifndef WIN32
#define WIN32
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define _WIN32_WINNT 0x0500
#else
#define _WIN32_WINNT 0x0501
#endif
#include <winsock2.h>
#include <windows.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>
#include "ZNetworkLibEvent.h"
#include "../../../../Core/Template/TAtomic.h"
#include <algorithm>
#include <sstream>


#ifdef __linux__
#include <arpa/inet.h>
#endif

using namespace PaintsNow;

struct ListenerLibEventImpl;
struct ConnectionLibEventImpl;

struct DispatcherLibEventImpl : public INetwork::Dispatcher {
	event_base* base;
	evdns_base* dns;
#ifdef _DEBUG
	std::atomic<int32_t> referCount;
#endif
};

#define BEV_EVENT_WAKEUP 0x1000

struct ListenerLibEventImpl : public INetwork::Listener {
	TWrapper<const TWrapper<void, ITunnel::Connection*, INetwork::EVENT>, INetwork::Connection*> callback;
	TWrapper<void, ITunnel::Listener*, INetwork::EVENT> eventHandler;
	std::set<ConnectionLibEventImpl*> autoConnections;
	DispatcherLibEventImpl* dispatcher;
	IThread* threadApi;
	evconnlistener* listener;
	event* ev;
	String ip;
	int port;
	bool isActivated;

	evhttp* http;
	TWrapper<void, INetwork::Connection*, INetwork::HttpRequest*> httpHandler;
};


struct ConnectionLibEventImpl : public INetwork::Connection {
	TWrapper<void, ITunnel::Connection*, INetwork::EVENT> callback;
	DispatcherLibEventImpl* dispatcher;
	ListenerLibEventImpl* listener;
	bufferevent* bev;
	String srcIP;
	String dstIP;
	String ip;
	int srcPort;
	int dstPort;
	int port;
	bool isActivated;
};

void UpdateConnectionInfo(ConnectionLibEventImpl* c, evutil_socket_t s) {
	sockaddr_in sock;
#if defined(WIN32) || defined(_WIN32)
	int len = sizeof(sock);
#else
	socklen_t len = sizeof(sock);
#endif

	::getsockname(s, (sockaddr*)&sock, &len);
	c->srcIP = ::inet_ntoa(sock.sin_addr);
	c->srcPort = htons(sock.sin_port);
	::getpeername(s, (sockaddr*)&sock, &len);
	c->dstIP = ::inet_ntoa(sock.sin_addr);
	c->dstPort = htons(sock.sin_port);
}

// sample code from : http://www.cnblogs.com/luxiaoxun/p/3603399.html

static void conn_writecb(struct bufferevent* bev, void* user_data) {
	ConnectionLibEventImpl* c = reinterpret_cast<ConnectionLibEventImpl*>(user_data);
	struct evbuffer* output = bufferevent_get_output(bev);
	// size_t length = evbuffer_get_length(output);
	if (evbuffer_get_length(output) == 0) {
		// printf("flushed answer\n");
		// bufferevent_free(bev);
	}

	c->callback(c, INetwork::WRITE);
}

static void conn_readcb(struct bufferevent* bev, void* user_data) {
	ConnectionLibEventImpl* c = reinterpret_cast<ConnectionLibEventImpl*>(user_data);
	struct evbuffer* input = bufferevent_get_input(bev);
	// size_t length = evbuffer_get_length(input);
	if (evbuffer_get_length(input) == 0) {
		// printf("flushed answer\n");
		// bufferevent_free(bev);
	} else {
		c->callback(c, INetwork::READ);
	}
}

static void conn_eventcb(struct bufferevent* bev, short events, void* user_data) {
	ConnectionLibEventImpl* c = reinterpret_cast<ConnectionLibEventImpl*>(user_data);

	if (events & BEV_EVENT_WAKEUP) {
		// printf("Connection awaked.\n");
		event_base* base = c->dispatcher->base;
		c->callback(c, INetwork::AWAKE);
		return;
	}

	if (events & BEV_EVENT_EOF) {
		// printf("Connection closed.\n");
		event_base* base = c->dispatcher->base;
		c->callback(c, INetwork::CLOSE);
		return;
	}

	if (events & BEV_EVENT_ERROR) {
		// printf("Got an error on the connection: %s\n",
		event_base* base = c->dispatcher->base;
		c->callback(c, INetwork::ABORT);
		return;
	}

	if (events & BEV_EVENT_CONNECTED) {
		UpdateConnectionInfo(c, bufferevent_getfd(bev));

		// bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, c);
		// bufferevent_enable(bev, EV_WRITE | EV_READ); // duplexing
		evutil_make_socket_nonblocking(bufferevent_getfd(c->bev));
		evutil_make_listen_socket_reuseable(bufferevent_getfd(c->bev));
		c->callback(c, INetwork::CONNECTED);
	}

	if (events & BEV_EVENT_TIMEOUT) {
		c->callback(c, INetwork::TIMEOUT);
	}

	if (events & BEV_EVENT_READING) {
		// c->callback(c, INetwork::READ);
	}

	if (events & BEV_EVENT_WRITING) {
		// c->callback(c, INetwork::WRITE);
	}

	/* None of the other events can happen here, since we haven't enabled
	* timeouts */
	// bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void* user_data) {
	struct ListenerLibEventImpl* p = (struct ListenerLibEventImpl*)user_data;
	// struct timeval delay = { 2, 0 };

	if (events & EV_TIMEOUT) {
		p->eventHandler(p, INetwork::TIMEOUT);
	}

	if (events & EV_READ) {
		p->eventHandler(p, INetwork::READ);
	}

	if (events & EV_WRITE) {
		p->eventHandler(p, INetwork::WRITE);
	}

	if (events & EV_SIGNAL) {
		p->eventHandler(p, INetwork::CLOSE);
	}

	/*
	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	*/
}

struct HttpRequestImpl : public INetwork::HttpRequest {
	evhttp_request* request;
	evhttp_connection* connection;
	ConnectionLibEventImpl* conimpl;
	TWrapper<void, int> callback;
	String uri;
	evhttp_cmd_type method;
	bool committed;
	bool ownConnection;
};

static void httpdcb(evhttp_request* request, void* userdata) {
	ListenerLibEventImpl* p = reinterpret_cast<ListenerLibEventImpl*>(userdata);
	HttpRequestImpl* r = new HttpRequestImpl();

	// overwrite connection info
	ConnectionLibEventImpl* c = new ConnectionLibEventImpl();
	evhttp_connection* conn = evhttp_request_get_connection(request);
	c->bev = evhttp_connection_get_bufferevent(conn);
	char* address;
	unsigned short port;
	evhttp_connection_get_peer(conn, &address, &port);

	c->srcIP = address;
	c->srcPort = port;
	c->dstIP = p->ip;
	c->dstPort = p->port;
	c->ip = c->srcIP;
	c->port = c->srcPort;
	c->isActivated = true;
	r->ownConnection = true;
	r->conimpl = c;
	c->dispatcher = p->dispatcher;
	c->listener = p;

#ifdef _DEBUG
	c->dispatcher->referCount.fetch_add(1, std::memory_order_relaxed);
#endif

	// bufferevent_setcb(c->bev, conn_readcb, conn_writecb, conn_eventcb, c);
	// bufferevent_enable(c->bev, EV_WRITE | EV_READ); // duplexing
	r->connection = conn;
	r->request = request;
	r->committed = true;
	p->httpHandler(r->conimpl, r);
	r->method = EVHTTP_REQ_GET;

	p->autoConnections.insert(c);
}

void FromHost(const String& host, String& ip, int& port) {
	const char* pos = strrchr(host.c_str(), ':');
	if (pos != nullptr) {
		ip = host.substr(0, pos - host.c_str());
		port = strtol(pos + 1, nullptr, 10);
	} else {
		ip = host;
		port = 0;
	}
}

void ToHost(String& host, const String& ip, int port) {
	std::ostringstream ss;
	ss << ip.c_str() << ":" << port;
	host = StdToUtf8(ss.str()).c_str();
}

static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data) {
	ListenerLibEventImpl* p = reinterpret_cast<ListenerLibEventImpl*>(user_data);
	struct bufferevent* bev;

	bev = bufferevent_socket_new(p->dispatcher->base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(p->dispatcher->base);
		return;
	}

	ConnectionLibEventImpl* c = new ConnectionLibEventImpl();
	c->bev = bev;
	sockaddr_in* in = (sockaddr_in*)sa;
	assert(socklen == sizeof(sockaddr_in));

	c->srcIP = inet_ntoa(in->sin_addr);
	c->srcPort = htons(in->sin_port);
	c->dstIP = p->ip;
	c->dstPort = p->port;
	c->bev = bev;
	c->ip = c->srcIP;
	c->port = c->srcPort;
	c->isActivated = true;
	c->dispatcher = p->dispatcher;
	c->listener = p;

#ifdef _DEBUG
	c->dispatcher->referCount.fetch_add(1, std::memory_order_relaxed);
#endif

	bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, c);
	bufferevent_enable(bev, EV_WRITE | EV_READ); // duplexing
	evutil_make_socket_nonblocking(bufferevent_getfd(bev));
	evutil_make_listen_socket_reuseable(bufferevent_getfd(bev));

	c->callback = p->callback(c);
	assert(c->callback);
	c->callback(c, ITunnel::CONNECTED);
	p->autoConnections.insert(c);
	// char MESSAGE[] = "Why?";
	// bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

ZNetworkLibEvent::ZNetworkLibEvent(IThread& t) : threadApi(t) {
#ifdef WIN32
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif

}

ZNetworkLibEvent::~ZNetworkLibEvent() {
#if defined(_MSC_VER) && _MSC_VER > 1200
	libevent_global_shutdown();
#endif
}

std::vector<String> ZNetworkLibEvent::Resolve(const String& name) {
	hostent* ent = ::gethostbyname(name.c_str());
	std::vector<String> addresses;
	if (ent != nullptr) {
		for (size_t i = 0; i < (size_t)ent->h_length; i++) {
			addresses.emplace_back(::inet_ntoa(*reinterpret_cast<in_addr*>(ent->h_addr_list[i])));
		}
	}

	return addresses;
}

INetwork::Listener* ZNetworkLibEvent::OpenListener(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Listener*, EVENT>& eventHandler, const TWrapper<const TWrapper<void, ITunnel::Connection*, EVENT>, Connection*>& callback, const String& host) {
	assert(dispatcher != nullptr);
	DispatcherLibEventImpl* disp = static_cast<DispatcherLibEventImpl*>(dispatcher);

	ListenerLibEventImpl* p = new ListenerLibEventImpl();
	p->callback = callback;
	String ip;
	int port;
	FromHost(host, ip, port);
	p->dispatcher = disp;
#ifdef _DEBUG
	p->dispatcher->referCount.fetch_add(1, std::memory_order_relaxed);
#endif
	p->ip = ip;
	p->port = port;
	p->threadApi = &threadApi;
	p->eventHandler = eventHandler;
	p->listener = nullptr;
	p->isActivated = false;
	p->http = nullptr;
	p->ev = nullptr;

	return p;
}

bool ZNetworkLibEvent::ActivateListener(Listener* listener) {
	ListenerLibEventImpl* p = static_cast<ListenerLibEventImpl*>(listener);
	IThread& threadApi = *p->threadApi;

	bool ret = false;
	if (p->listener == nullptr) {
		sockaddr_in s;
		s.sin_family = AF_INET;
		s.sin_port = htons(p->port);
		s.sin_addr.s_addr = inet_addr(p->ip.c_str());

		p->listener = evconnlistener_new_bind(p->dispatcher->base, listener_cb, p, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (const sockaddr*)&s, sizeof(s));
		if (p->listener != nullptr) {
			// any port
			if (p->port == 0) {
#if defined(WIN32) || defined(_WIN32)
				int len = sizeof(s);
#else
				socklen_t len = sizeof(s);
#endif
				::getsockname(evconnlistener_get_fd(p->listener), (sockaddr*)&s, &len);
				p->port = htons(s.sin_port);
			}

			evutil_make_socket_nonblocking(evconnlistener_get_fd(p->listener));
			p->ev = event_new(p->dispatcher->base, evconnlistener_get_fd(p->listener), EV_TIMEOUT | EV_READ | EV_WRITE | EV_PERSIST, signal_cb, (void*)p);

			// event_add(p->ev, nullptr);

			if (p->httpHandler) {
				p->http = evhttp_new(p->dispatcher->base);
				evhttp_set_gencb(p->http, httpdcb, p);
				evhttp_bind_listener(p->http, p->listener);
			}

			p->eventHandler(p, INetwork::CONNECTED);
			ret = true;
		}
	}

	p->isActivated = true;
	return ret;
}

void ZNetworkLibEvent::SetListenerForHttpd(Listener* l, const TWrapper<void, Connection*, HttpRequest*>& httpRequestHandler) {
	ListenerLibEventImpl* listener = static_cast<ListenerLibEventImpl*>(l);
	listener->httpHandler = httpRequestHandler;
}

void ZNetworkLibEvent::DeactivateListener(Listener* l) {
	ListenerLibEventImpl* listener = static_cast<ListenerLibEventImpl*>(l);
	if (listener->isActivated) {
		listener->isActivated = false;

		// Close all auto connections
		std::set<ConnectionLibEventImpl*> connections;
		std::swap(connections, listener->autoConnections);
		for (std::set<ConnectionLibEventImpl*>::iterator c = connections.begin(); c != connections.end(); ++c) {
			if ((*c)->callback) {
				(*c)->callback(*c, ITunnel::ABORT);
			}
		}

		if (listener->http != nullptr) {
			evhttp_free(listener->http);
		} else if (listener->listener != nullptr) {
			evconnlistener_free(listener->listener);
			listener->listener = nullptr;
		}

		if (listener->ev != nullptr) {
			event_free(listener->ev);
		}
	}
}

String ZNetworkLibEvent::GetAddress(Listener* listener) {
	ListenerLibEventImpl* impl = static_cast<ListenerLibEventImpl*>(listener);
	String info;
	ToHost(info, impl->ip, impl->port);

	return info;
}

void ZNetworkLibEvent::CloseListener(Listener* l) {
	ListenerLibEventImpl* listener = static_cast<ListenerLibEventImpl*>(l);
	DeactivateListener(listener);
#ifdef _DEBUG
	listener->dispatcher->referCount.fetch_sub(1, std::memory_order_relaxed);
#endif
	delete listener;
}

INetwork::Connection* ZNetworkLibEvent::OpenConnection(Dispatcher* dispatcher, const TWrapper<void, ITunnel::Connection*, EVENT>& callback, const String& host) {
	assert(dispatcher != nullptr);
	DispatcherLibEventImpl* disp = static_cast<DispatcherLibEventImpl*>(dispatcher);
	ConnectionLibEventImpl* p = new ConnectionLibEventImpl();
	String ip;
	int port;
	FromHost(host, ip, port);
	p->dispatcher = disp;
	p->listener = nullptr;
#ifdef _DEBUG
	p->dispatcher->referCount.fetch_add(1, std::memory_order_relaxed);
#endif
	p->callback = callback;
	p->ip = ip;
	p->port = port;
	p->bev = nullptr;
	p->isActivated = false;

	p->bev = bufferevent_socket_new(p->dispatcher->base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
	bufferevent_setcb(p->bev, conn_readcb, conn_writecb, conn_eventcb, p);
	bufferevent_enable(p->bev, EV_READ | EV_WRITE);

	return p;
}

void ZNetworkLibEvent::SetDispatcherOption(Dispatcher* dispatcher, const String& option, size_t value) {
	// TODO:
}

bool ZNetworkLibEvent::ActivateConnection(Connection* c) {
	ConnectionLibEventImpl* p = static_cast<ConnectionLibEventImpl*>(c);
	if (!p->isActivated) {
		return p->isActivated = (bufferevent_socket_connect_hostname(p->bev, p->dispatcher->dns, AF_UNSPEC, p->ip.c_str(), p->port) != 0);
	} else {
		return true;
	}
}

void ZNetworkLibEvent::DeactivateConnection(Connection* c) {
	ConnectionLibEventImpl* connection = static_cast<ConnectionLibEventImpl*>(c);
	if (connection->isActivated) {
		assert(connection->bev != nullptr);
		evutil_closesocket(bufferevent_getfd(connection->bev));
		connection->isActivated = false;
	}
}

void ZNetworkLibEvent::Flush(Connection* c) {
	ConnectionLibEventImpl* connection = static_cast<ConnectionLibEventImpl*>(c);
	bufferevent_flush(connection->bev, EV_WRITE, BEV_FLUSH);
}

struct PacketHeader {
	uint32_t length;
	uint32_t checksum;
};

bool ZNetworkLibEvent::WriteConnection(Connection* c, const void* data, size_t length, size_t mode) {
	assert(length != 0);
	ConnectionLibEventImpl* connection = static_cast<ConnectionLibEventImpl*>(c);
	if (mode & ITunnel::STREAMED) {
		return bufferevent_write(connection->bev, data, length) == 0;
	} else {
		PacketHeader header;
		header.length = verify_cast<uint32_t>(length);
		header.checksum = (uint32_t)HashBuffer(&header.length, sizeof(uint32_t));
		return bufferevent_write(connection->bev, &header, sizeof(header)) == 0 && bufferevent_write(connection->bev, data, length) == 0;
	}
}

bool ZNetworkLibEvent::ReadConnection(Connection* c, void* data, size_t& length, size_t& mode) {
	ConnectionLibEventImpl* connection = static_cast<ConnectionLibEventImpl*>(c);
	if (mode & ITunnel::STREAMED) {
		if (data == nullptr) {
			evbuffer* buffer = bufferevent_get_input(connection->bev);
			length = evbuffer_get_length(buffer);
		} else {
			length = bufferevent_read(connection->bev, data, length);
		}

		return true;
	} else {
		evbuffer* buffer = bufferevent_get_input(connection->bev);
		PacketHeader header;
		if (evbuffer_copyout(buffer, &header, sizeof(header)) != sizeof(header)) {
			return false;
		}

		uint32_t checksum = (uint32_t)HashBuffer(&header.length, sizeof(uint32_t));
		if (header.checksum != checksum) {
			DeactivateConnection(c); // connection failed!
			return false;
		}

		uint32_t size = header.length;
		if (data == nullptr) {
			length = size;
			return true;
		} else if (length >= size) {
			size_t valid = evbuffer_get_length(buffer);
			if (valid >= length + sizeof(header)) { // enough
				bufferevent_read(connection->bev, &header, sizeof(header));
				length = bufferevent_read(connection->bev, data, size);
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
}

void ZNetworkLibEvent::GetConnectionAddresses(INetwork::Connection* connection, String& src, String& dst) {
	ConnectionLibEventImpl* c = static_cast<ConnectionLibEventImpl*>(connection);
	ToHost(src, c->srcIP, c->srcPort);
	ToHost(dst, c->dstIP, c->dstPort);
}

void ZNetworkLibEvent::CloseConnection(Connection* c) {
	ConnectionLibEventImpl* connection = static_cast<ConnectionLibEventImpl*>(c);
	DeactivateConnection(connection);
	bufferevent_free(connection->bev);

	if (connection->listener != nullptr)
		connection->listener->autoConnections.erase(connection);

#ifdef _DEBUG
	connection->dispatcher->referCount.fetch_sub(1, std::memory_order_relaxed);
#endif
	delete connection;
}

static void httpcb(evhttp_request* request, void* userdata) {
	HttpRequestImpl* p = static_cast<HttpRequestImpl*>(userdata);

	bufferevent* bev = evhttp_connection_get_bufferevent(p->connection);
	evutil_socket_t s = bufferevent_getfd(bev);
	UpdateConnectionInfo(p->conimpl, s);
	evutil_make_listen_socket_reuseable(s);

	p->callback(p->request->response_code);
}

void ZNetworkLibEvent::PrepareHttpRequest(HttpRequest* req) {
	HttpRequestImpl* p = static_cast<HttpRequestImpl*>(req);
	if (p->request != nullptr && !p->committed) {
		// int owned = evhttp_request_is_owned(p->request);
		evhttp_request_free(p->request);
	}

	p->request = evhttp_request_new(httpcb, p);
	p->committed = false;
	// evhttp_request_own(p->request);
}

INetwork::HttpRequest* ZNetworkLibEvent::OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback) {
	ConnectionLibEventImpl* con = static_cast<ConnectionLibEventImpl*>(connection);
	HttpRequestImpl* p = new HttpRequestImpl();
	p->request = nullptr;
	p->conimpl = con;
	p->committed = false;
	p->ownConnection = false;
	// well evhttp_connection_base_bufferevent_new() will be supported in next version of libevent2
	// here we create a new one instead.
	// just wait for a new release of libevent ...
	// p->connection = evhttp_connection_base_bufferevent_new(base, dns, con->bev, con->dstIP.c_str(), con->dstPort);
	p->connection = evhttp_connection_base_new(con->dispatcher->base, con->dispatcher->dns, con->ip.c_str(), con->port);

#ifdef _DEBUG
	con->dispatcher->referCount.fetch_add(1, std::memory_order_relaxed);
#endif

	p->callback = callback;
	p->method = EVHTTP_REQ_GET;

	return p;
}

void ZNetworkLibEvent::CloseHttpRequest(INetwork::HttpRequest* request) {
	HttpRequestImpl* p = static_cast<HttpRequestImpl*>(request);
#ifdef _DEBUG
	p->conimpl->dispatcher->referCount.fetch_sub(1, std::memory_order_relaxed);
#endif
	if (p->ownConnection) {
		delete p->conimpl;
	} else {
		evhttp_connection_free(p->connection);
	}

	if (p->request != nullptr && !p->committed) {
		evhttp_request_free(p->request);
	}

	delete p;
}

String ZNetworkLibEvent::GetHttpRequestUri(HttpRequest* request) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	return evhttp_request_get_uri(r->request);
}

void ZNetworkLibEvent::SetHttpRequestUri(HttpRequest* request, const String& uri) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	r->uri = uri;
	assert(r->request != nullptr);
}

String ZNetworkLibEvent::GetHttpRequestMethod(HttpRequest* request) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evhttp_cmd_type type = evhttp_request_get_command(r->request);
	switch (type) {
		case EVHTTP_REQ_GET:
			return "GET";
		case EVHTTP_REQ_POST:
			return "POST";
		case EVHTTP_REQ_HEAD:
			return "HEAD";
		case EVHTTP_REQ_PUT:
			return "PUT";
		case EVHTTP_REQ_DELETE:
			return "DELETE";
		case EVHTTP_REQ_OPTIONS:
			return "OPTIONS";
		case EVHTTP_REQ_CONNECT:
			return "CONNECT";
		case EVHTTP_REQ_PATCH:
			return "PATCH";
		default:
			return "GET";
	}

	return "GET";
}

void ZNetworkLibEvent::SetHttpRequestMethod(HttpRequest* request, const String& method) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	String m = method;
	/*
	std::transform(method.begin(), method.end(), m.begin(), toupper);
	*/

	for (size_t i = 0; i < m.length(); i++) {
		m[i] = toupper(m[i]);
	}

	if (m == "GET") {
		r->method = EVHTTP_REQ_GET;
	} else if (m == "POST") {
		r->method = EVHTTP_REQ_POST;
	} else if (m == "HEAD") {
		r->method = EVHTTP_REQ_HEAD;
	} else if (m == "PUT") {
		r->method = EVHTTP_REQ_PUT;
	} else if (m == "DELETE") {
		r->method = EVHTTP_REQ_DELETE;
	} else if (m == "OPTIONS") {
		r->method = EVHTTP_REQ_OPTIONS;
	} else if (m == "CONNECT") {
		r->method = EVHTTP_REQ_CONNECT;
	} else if (m == "PATCH") {
		r->method = EVHTTP_REQ_PATCH;
	} else {
		r->method = EVHTTP_REQ_GET;
	}
}

static void FromList(evkeyvalq* q, const std::list<std::pair<String, String> >& header) {
	for (std::list<std::pair<String, String> >::const_iterator it = header.begin(); it != header.end(); ++it) {
		evhttp_add_header(q, (*it).first.c_str(), (*it).second.c_str());
	}
}

static void ToList(evkeyvalq* kv, std::list<std::pair<String, String> >& header) {
	if (kv->tqh_last != nullptr) {
		evkeyval* q = kv->tqh_first;
		while (q != *kv->tqh_last) {
			header.emplace_back(std::make_pair(String(q->key), String(q->value)));
			q = q->next.tqe_next;
		}
	}
}

void ZNetworkLibEvent::GetHttpRequestHeader(HttpRequest* request, std::list<std::pair<String, String> >& header) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evkeyvalq* q = evhttp_request_get_input_headers(r->request);
	ToList(q, header);
}

void ZNetworkLibEvent::SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evkeyvalq* q = evhttp_request_get_output_headers(r->request);
	FromList(q, header);
}

void ZNetworkLibEvent::ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment) {
	evhttp_uri* u = evhttp_uri_parse(uri.c_str());
	const char* userinfo = evhttp_uri_get_userinfo(u);
	if (userinfo != nullptr)
		user = userinfo;
	const char* hostinfo = evhttp_uri_get_host(u);
	if (hostinfo != nullptr)
		host = hostinfo;

	port = evhttp_uri_get_port(u);
	if (port == -1) port = 80;

	const char* fraginfo = evhttp_uri_get_fragment(u);
	if (fraginfo != nullptr)
		fragment = fraginfo;

	const char* pathinfo = evhttp_uri_get_path(u);
	if (pathinfo != nullptr)
		path = pathinfo;
	const char* e = evhttp_uri_get_query(u);
	evkeyvalq kv;
	evhttp_parse_query_str(e, &kv);
	ToList(&kv, query);

	evhttp_clear_headers(&kv);
	evhttp_uri_free(u);
}

String ZNetworkLibEvent::MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment) {
	String ret;
	evhttp_uri* u = evhttp_uri_new();
	evhttp_uri_set_userinfo(u, user.c_str());
	String ip;
	int port;
	FromHost(host, ip, port);
	evhttp_uri_set_host(u, host.c_str());
	evhttp_uri_set_port(u, port);
	String str;
	for (std::list<std::pair<String, String> >::const_iterator it = query.begin(); it != query.end(); ++it) {
		if (it != query.begin()) {
			str += "&";
		}
		char* h = evhttp_encode_uri((*it).first.c_str());
		str += h;
		str += "=";
		free(h);
		h = evhttp_encode_uri((*it).second.c_str());
		str += h;
		free(h);
	}

	evhttp_uri_set_path(u, path.c_str());
	evhttp_uri_set_fragment(u, fragment.c_str());
	evhttp_uri_set_query(u, str.c_str());
	ret.resize((path.length() + fragment.length() + str.length()) * 6 + 24);
	evhttp_uri_join(u, (char*)ret.data(), ret.size());
	evhttp_uri_free(u);

	return String(ret.c_str());
}

void ZNetworkLibEvent::MakeHttpRequest(HttpRequest* request) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	String uri = r->uri;
	// drop http://xxxx@xxx.com part
	String::size_type pos = uri.find("//");
	if (pos != String::npos) {
		pos = uri.find('/', pos + 2);
		if (pos != String::npos) {
			uri = uri.substr(pos);
		}
	}

	evhttp_add_header(r->request->output_headers, "Host", r->conimpl->ip.c_str());
	evhttp_make_request(r->connection, r->request, r->method, uri.c_str());
	r->committed = true;
}

void ZNetworkLibEvent::MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evbuffer* buffer = evbuffer_new();
	evbuffer_add(buffer, data.data(), data.length());
	evhttp_send_reply(r->request, code, reason.c_str(), buffer);
	evbuffer_free(buffer);
}

String ZNetworkLibEvent::GetHttpRequestData(HttpRequest* request) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evbuffer* buffer = evhttp_request_get_input_buffer(r->request);
	size_t size = evbuffer_get_length(buffer);
	String ret;
	ret.resize(size);
	evbuffer_copyout(buffer, (char*)ret.data(), size);

	return ret;
}

void ZNetworkLibEvent::SetHttpRequestData(HttpRequest* request, const String& data) {
	HttpRequestImpl* r = static_cast<HttpRequestImpl*>(request);
	assert(r->request != nullptr);
	evbuffer* buffer = evhttp_request_get_output_buffer(r->request);
	evbuffer_add(buffer, (char*)data.data(), data.size());
}

ITunnel::Dispatcher* ZNetworkLibEvent::OpenDispatcher() {
	DispatcherLibEventImpl* p = new DispatcherLibEventImpl();
	p->base = event_base_new();
	p->dns = evdns_base_new(p->base, 1);
#ifdef _DEBUG
	p->referCount = 0;
#endif
	evthread_make_base_notifiable(p->base);
	return p;
}

bool ZNetworkLibEvent::ActivateDispatcher(Dispatcher* dispatcher) {
	assert(dispatcher != nullptr);
	DispatcherLibEventImpl* impl = static_cast<DispatcherLibEventImpl*>(dispatcher);
	event_base_dispatch(impl->base);
	return true;
}

void ZNetworkLibEvent::DeactivateDispatcher(Dispatcher* dispatcher) {
	assert(dispatcher != nullptr);
	DispatcherLibEventImpl* impl = static_cast<DispatcherLibEventImpl*>(dispatcher);
	event_base_loopbreak(impl->base);
}

ITunnel::Dispatcher* ZNetworkLibEvent::GetListenerDispatcher(Listener* listener) {
	assert(listener != nullptr);
	ListenerLibEventImpl* impl = static_cast<ListenerLibEventImpl*>(listener);
	return impl->dispatcher;
}

ITunnel::Dispatcher* ZNetworkLibEvent::GetConnectionDispatcher(Connection* connection) {
	assert(connection != nullptr);
	ConnectionLibEventImpl* impl = static_cast<ConnectionLibEventImpl*>(connection);
	return impl->dispatcher;
}

void ZNetworkLibEvent::CloseDispatcher(Dispatcher* dispatcher) {
	assert(dispatcher != nullptr);
	DispatcherLibEventImpl* impl = static_cast<DispatcherLibEventImpl*>(dispatcher);
#ifdef _DEBUG
	assert(impl->referCount.load() == 0);
#endif
	evdns_base_free(impl->dns, 1);
	event_base_free(impl->base);
	delete impl;
}