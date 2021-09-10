#include "../../Core/Template/TBuffer.h"
#include "../../Core/System/MemoryStream.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"
#include "RemoteProxy.h"
#include <iterator>

using namespace PaintsNow;

enum { UNIQUE_GLOBAL = 0 };
enum { GLOBAL_INTERFACE_CREATE = 0, GLOBAL_INTERFACE_QUERY };

void Value<TableImpl>::Reflect(IReflect& reflect) {
	if (reflect.IsReflectProperty()) {
		ReflectProperty(value.arrayPart);
		uint64_t count = value.mapPart.size();
		ReflectProperty(count);
		if (value.mapPart.size() == 0) { // Read
			for (size_t i = 0; i < count; i++) {
				Variant v;
				String key;
				reflect.OnProperty(key, "Key", this, &key, nullptr);
				reflect.OnProperty(v, "Value", this, &v, nullptr);
				value.mapPart[key] = v;
			}
		} else {
			for (std::map<String, Variant>::iterator it = value.mapPart.begin(); it != value.mapPart.end(); ++it) {
				reflect.OnProperty(it->first, "Key", this, (void*)&it->first, nullptr);
				reflect.OnProperty(it->second, "Value", this, &it->second, nullptr);
			}
		}
	}
}

TObject<IReflect>& Variant::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		char type = value == nullptr ? TYPE_INT64 : value->GetType();
		ReflectProperty(type);
		assert(type != TYPE_ERROR);
		if (value == nullptr) {
			switch (type) {
				case TYPE_INT8:
					value = Construct((int8_t)(0));
					break;
				case TYPE_BOOL:
					value = Construct((bool)false);
					break;
				case TYPE_INT16:
					value = Construct((int16_t)(0));
					break;
				case TYPE_INT32:
					value = Construct((int32_t)(0));
					break;
				case TYPE_INT64:
					value = Construct((int64_t)(0));
					break;
				case TYPE_FLOAT:
					value = Construct((float)(0.0f));
					break;
				case TYPE_DOUBLE:
					value = Construct((double)(0.0f));
					break;
				case TYPE_STRING:
					value = Construct((String)(""));
					break;
				case TYPE_OBJECT:
					value = Construct(IScript::BaseDelegate(nullptr));
					break;
				case TYPE_TABLE:
					value = Construct(TableImpl());
					break;
			}
		}

		if (value != nullptr)
			value->Reflect(reflect);
	}

	return *this;
}

RemoteProxy::Request::Packet::Packet() : object(0), procedure(0), callback(0), response(false) {}

TObject<IReflect>& RemoteProxy::Request::Packet::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(object);
		ReflectProperty(procedure);
		ReflectProperty(callback);
		ReflectProperty(response);
		ReflectProperty(deferred);
		ReflectProperty(vars);
		ReflectProperty(localDelta);
		ReflectProperty(remoteDelta);
	}

	return *this;
}

TObject<IReflect>& RemoteProxy::operator() (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

class MethodInspector : public IReflect {
public:
	MethodInspector(IScript::Object* o, RemoteProxy::ObjectInfo& info) : IReflect(false, true), object(o), objectInfo(info) {}

	~MethodInspector() override {
		for (size_t i = 0; i < objectInfo.collection.size(); i++) {
			objectInfo.collection[i].method = Wrap(&objectInfo.collection[i], &RemoteProxy::ObjectInfo::Entry::CallFilter);
			objectInfo.collection[i].index = (long)i;
		}

		objectInfo.needQuery = false;
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {}
	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaMethod::TypedBase>::Get();
		for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
			const MetaNodeBase* node = t->GetNode();
			if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
				const IScript::MetaMethod::TypedBase* entry = static_cast<const IScript::MetaMethod::TypedBase*>(node);
				objectInfo.collection.emplace_back(RemoteProxy::ObjectInfo::Entry());
				RemoteProxy::ObjectInfo::Entry& t = objectInfo.collection.back();
				t.name = entry->name.empty() ? name : entry->name;
				t.retValue = retValue;
				t.params = params;
				t.wrapper = entry->CreateWrapper();
				t.obj = object;
			}
		}
	}

	IScript::Object* object;
	RemoteProxy::ObjectInfo& objectInfo;
};

RemoteProxy::RemoteProxy(IThread& threadApi, ITunnel& t, const TWrapper<IScript::Object*, const String&>& creator, const String& entry, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& sh) : IScript(threadApi), tunnel(t), defaultRequest(*this, nullptr, statusHandler), objectCreator(creator), statusHandler(sh), dispatcher(nullptr) {
	SetEntry(entry);
	dispThread.store(nullptr, std::memory_order_relaxed);
}

void RemoteProxy::Stop() {
	if (dispatcher != nullptr) {
		tunnel.DeactivateDispatcher(dispatcher);

		IThread::Thread* thread = dispThread.exchange(nullptr, std::memory_order_release);
		if (thread != nullptr) {
			threadApi.Wait(thread);
			threadApi.DeleteThread(thread);
		}
	}
}

RemoteProxy::~RemoteProxy() {
	Stop();

	if (dispatcher != nullptr) {
		tunnel.CloseDispatcher(dispatcher);
	}
}

const TWrapper<IScript::Object*, const String&>& RemoteProxy::GetObjectCreator() const {
	return objectCreator;
}

bool RemoteProxy::ThreadProc(IThread::Thread* thread, size_t context) {
	OPTICK_THREAD("Network RemoteProxy");

	ITunnel::Dispatcher* disp = dispatcher;
	assert(!entry.empty());
	ITunnel::Listener* listener = tunnel.OpenListener(disp, Wrap(this, &RemoteProxy::HandleEvent), Wrap(this, &RemoteProxy::OnConnection), entry);
	if (listener == nullptr) {
		HandleEvent(ITunnel::ABORT);
		return false;
	}

	tunnel.ActivateListener(listener);
	tunnel.ActivateDispatcher(disp); // running ...

	tunnel.DeactivateListener(listener);
	tunnel.CloseListener(listener);

	IThread::Thread* t = dispThread.exchange(nullptr, std::memory_order_release);
	if (t != nullptr) {
		assert(t == thread);
		threadApi.DeleteThread(t);
	}

	return false;
}

void RemoteProxy::HandleEvent(ITunnel::EVENT event) {}

void RemoteProxy::SetEntry(const String& e) {
	entry = e;
}

void RemoteProxy::Reset() {
	Stop();
	Run();
}

bool RemoteProxy::Run() {
	if (dispatcher == nullptr) {
		dispatcher = tunnel.OpenDispatcher();
	}

	assert(dispThread.load(std::memory_order_relaxed) == nullptr);
	dispThread.store(threadApi.NewThread(Wrap(this, &RemoteProxy::ThreadProc), 0), std::memory_order_relaxed);

	return dispThread.load(std::memory_order_relaxed) != nullptr;
}

void RemoteProxy::Reconnect(IScript::Request& request) {
	RemoteProxy::Request& req = static_cast<RemoteProxy::Request&>(request);
	req.Reconnect();
}

void RemoteProxy::Request::Run() {
	host.tunnel.ActivateConnection(connection);
}

const ITunnel::Handler RemoteProxy::OnConnection(ITunnel::Connection* connection) {
	RemoteProxy::Request* request = new RemoteProxy::Request(*this, connection, statusHandler);
	request->Attach(connection);
	return Wrap(request, &Request::OnConnection);
}

IScript::Request* RemoteProxy::NewRequest(const String& entry) {
	Request* request = new RemoteProxy::Request(*this, nullptr, statusHandler);
	request->manually = true;
	assert(dispatcher != nullptr);

	ITunnel::Connection* c = tunnel.OpenConnection(dispatcher, Wrap(request, &Request::OnConnection), entry);
	if (c != nullptr) {
		request->Attach(c);
		request->Run();
		return request;
	} else {
		request->Destroy();
		return nullptr;
	}
}

IScript::Request& RemoteProxy::GetDefaultRequest() {
	return defaultRequest;
}

IScript::Request& RemoteProxy::Request::CleanupIndex() {
	idx = initCount;
	return *this;
}

RemoteProxy::ObjectInfo::Entry::Entry(const Entry& rhs) {
	*this = rhs;
}

RemoteProxy::ObjectInfo::Entry::Entry() : wrapper(nullptr), index(0) {
	
}

RemoteProxy::ObjectInfo::Entry::~Entry() {
	if (wrapper != nullptr) {
		delete wrapper;
	}
}

RemoteProxy::ObjectInfo::Entry& RemoteProxy::ObjectInfo::Entry::operator = (const Entry& rhs) {
	name = rhs.name;
	retValue = rhs.retValue;
	params = rhs.params;
	wrapper = rhs.wrapper == nullptr ? nullptr : rhs.wrapper->Clone();
	method = rhs.method;
	obj = rhs.obj;
	index = rhs.index;

	return *this;
}

RemoteProxy::ObjectInfo::ObjectInfo() : refCount(0), needQuery(true) {}
RemoteProxy::ObjectInfo::~ObjectInfo() {}

void RemoteProxy::Request::Attach(ITunnel::Connection* c) {
	connection = c;
}

void RemoteProxy::Request::PostPacket(Packet& packet) {

	/*
	// register objects if exists
	for (size_t i = packet.start; i < packet.vars.size(); i++) {
		Variant& var = packet.vars[i];
		if (var->QueryValueUnique() == UniqueType<IScript::Object*>::Get()) {
			Request::Value<IScript::Object*>& v = static_cast<Request::Value<IScript::Object*>&>(*var.Get());
			// inspect routines
			std::map<IScript::Object*, ObjectInfo>::iterator it = removedRemoteObjects.find(v.value);
			if (it == activeObjects.end()) {
				ObjectInfo& info = (activeObjects[v.value] = ObjectInfo());
				MethodInspector inspector(info);
				(*v.value)(inspector);
			}
		}
	}*/

	// swap local <=> remote
	for (std::map<IScript::Object*, size_t>::iterator it = remoteObjectRefDelta.begin(); it != remoteObjectRefDelta.end(); ++it) {
		packet.localDelta.emplace_back(std::make_pair((int64_t)it->first, it->second));
	}

	for (std::map<IScript::Object*, size_t>::iterator is = localObjectRefDelta.begin(); is != localObjectRefDelta.end(); ++is) {
		packet.remoteDelta.emplace_back(std::make_pair((int64_t)is->first, is->second));
	}

	// use filter 
	// *connection << packet;
	MemoryStream stream(0x1000);
	stream << packet;
	ITunnel::Packet state;
	state.header.length = (ITunnel::PacketSizeType)stream.GetTotalLength();
	host.tunnel.WriteConnectionPacket(connection, stream.GetBuffer(), verify_cast<uint32_t>(stream.GetTotalLength()), state);
	host.tunnel.Flush(connection);
	remoteObjectRefDelta.clear();
	localObjectRefDelta.clear();
}

void RemoteProxy::Request::ApplyDelta(std::map<IScript::Object*, ObjectInfo>& info, const std::vector<std::pair<int64_t, int64_t> >& delta, bool retrieve) {
	for (std::vector<std::pair<int64_t, int64_t> >::const_iterator it = delta.begin(); it != delta.end(); ++it) {
		const std::pair<int64_t, int64_t>& value = *it;
		// merge edition
		IScript::Object* object = reinterpret_cast<IScript::Object*>(value.first);
		assert(object != nullptr);
		std::map<IScript::Object*, ObjectInfo>::iterator p = info.find(object);
		if (p != info.end()) {
			if ((p->second.refCount += value.second) <= 0) {
				// destroy
				IScript::Object* object = reinterpret_cast<IScript::Object*>(value.first);
				info.erase(p);
				if (retrieve) { // local object
					object->ScriptUninitialize(*this);
				}
			}
		} else {
			ObjectInfo& x = info[object];
			if (retrieve) { // local object
				MethodInspector ins(object, x);
				(*object)(ins);
			}

			x.refCount = value.second;
			object->ScriptInitialize(*this);
		}
	}
}

void RemoteProxy::Request::ProcessPacket(Packet& packet) {
	DoLock();
	ApplyDelta(localActiveObjects, packet.localDelta, true);
	ApplyDelta(remoteActiveObjects, packet.remoteDelta, false);
	packet.localDelta.clear();
	packet.remoteDelta.clear();

	// lookup wrapper
	IScript::Request::AutoWrapperBase* wrapper = nullptr;
	bool needFree = false;

	if (packet.response) {
		if (packet.deferred) {
			IScript::Request::AutoWrapperBase* temp = reinterpret_cast<IScript::Request::AutoWrapperBase*>(packet.callback);
			std::set<IScript::Request::AutoWrapperBase*>::iterator it = localCallbacks.find(temp);
			if (it != localCallbacks.end()) {
				wrapper = temp;
				localCallbacks.erase(it);
				needFree = true;
			}
		}
	} else {
		int64_t proc = packet.procedure;
		if (packet.object == UNIQUE_GLOBAL) {
			if (proc < (int64_t)globalRoutines.collection.size()) {
				wrapper = globalRoutines.collection[(size_t)proc].wrapper;
			}
		} else {
			// retrieve object
			IScript::Object* object = reinterpret_cast<IScript::Object*>(packet.object);
			std::map<IScript::Object*, ObjectInfo>::iterator it = localActiveObjects.find(object);
			if (it != localActiveObjects.end()) {
				if (proc < (int64_t)it->second.collection.size()) {
					wrapper = it->second.collection[(size_t)proc].wrapper;
				}
			}
		}
	}

	// write values back
	if (!packet.deferred && (wrapper == nullptr || wrapper->IsSync())) {
		idx = (int)buffer.size();
		std::copy(packet.vars.begin(), packet.vars.end(), std::back_inserter(buffer));
		threadApi.Signal(syncCallEvent);
	} else if (wrapper != nullptr) {
		assert(!wrapper->IsSync());
		int argCount = (int)packet.vars.size();
		// push vars
		int saveIdx = idx;
		int saveInitCount = initCount;
		int saveTableLevel = tableLevel;
		idx = initCount = tableLevel = 0;
		std::swap(packet.vars, buffer);
		wrapper->Execute(*this);

		// pop vars
		// write results back
		std::swap(packet.vars, buffer);
		std::vector<Variant> v;
		std::copy(packet.vars.begin() + argCount, packet.vars.end(), std::back_inserter(v));
		std::swap(packet.vars, v);

		idx = saveIdx;
		initCount = saveInitCount;
		tableLevel = saveTableLevel;

		// write back?
		if (!packet.response) {
			packet.response = true;
			UnLock();
			PostPacket(packet);
			DoLock();
		}
	}

	if (needFree) {
		delete wrapper;
	}

	UnLock();
}

void RemoteProxy::Request::Process() {
	ITunnel::PacketSizeType bufferLength = CHUNK_SIZE;
	Bytes current;
	current.Resize(bufferLength);

	while (host.tunnel.ReadConnectionPacket(connection, current.GetData(), bufferLength, state)) {
		size_t len = bufferLength;
		if (!stream.WriteBlock(current.GetData(), len)) {
			stream.Seek(IStreamBase::BEGIN, 0);
			break;
		}

		if (state.cursor == state.header.length) {
			stream.Seek(IStreamBase::BEGIN, 0);
			Request::Packet packet;
			if (!(stream >> packet)) {
				return;
			}

			ProcessPacket(packet);
			stream.Seek(IStreamBase::BEGIN, 0);
		}

		bufferLength = CHUNK_SIZE;
	}
}

void RemoteProxy::Request::OnConnection(ITunnel::EVENT event) {
	// { CONNECTED, TIMEOUT, READ, WRITE, CLOSE, ERROR }
	switch (event) {
		case ITunnel::CONNECTED:
			if (statusHandler)
				statusHandler(*this, !manually, RemoteProxy::CONNECTED, "Connection established");
			break;
		case ITunnel::TIMEOUT:
			if (statusHandler)
				statusHandler(*this, !manually, RemoteProxy::TIMEOUT, "Connection timeout");
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
					statusHandler(*this, !manually, RemoteProxy::CLOSED, "Connection closed");
				} else {
					statusHandler(*this, !manually, RemoteProxy::ABORTED, "Connection aborted");
				}
			}

			host.tunnel.CloseConnection(connection);
			connection = nullptr;
			if (!manually) {
				delete this;
			}
			break;
		case ITunnel::CUSTOM:
			break;
	}
}

ValueBase::ValueBase() {}

ValueBase::~ValueBase() {}

TObject<IReflect>& RemoteProxy::Request::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewObject)[ScriptMethod = "NewObject"];
		ReflectMethod(RequestQueryObject)[ScriptMethod = "QueryObject"];
	}

	return *this;
}

Variant::~Variant() {
	Destroy();
}

Variant::Variant(const Variant& var) : value(nullptr) {
	*this = var;
}

Variant& Variant::operator = (const Variant& var) {
	if (&var != this) {
		// add reference
		if (value != nullptr) {
			Destroy();
		}

		value = var.value;

		if (value != nullptr) {
			size_t* ref2 = (size_t*)((char*)value - sizeof(size_t));
			assert(*ref2 < 0x100);
			AddRef();
		}
	}

	return *this;
}

// Request Apis

RemoteProxy::Request::Request(RemoteProxy& h, ITunnel::Connection* c, const TWrapper<void, IScript::Request&, bool, STATUS, const String&>& sh) : host(h), threadApi(host.threadApi), stream(CHUNK_SIZE, true), statusHandler(sh), idx(0), initCount(0), tableLevel(0), connection(c), manually(false), isKey(false) {
	syncCallEvent = threadApi.NewEvent();
	MethodInspector inspector((IScript::Object*)UNIQUE_GLOBAL, globalRoutines);
	(*this)(inspector);
}

void RemoteProxy::Request::Reconnect() {
	String sip, dip;
	host.tunnel.GetConnectionInfo(connection, sip, dip);
	host.tunnel.CloseConnection(connection);
	stream.Seek(IStreamBase::BEGIN, 0);
	connection = host.tunnel.OpenConnection(host.dispatcher, Wrap(this, &Request::OnConnection), dip);
}

RemoteProxy::Request::~Request() {
	if (manually && connection != nullptr) {
		host.tunnel.CloseConnection(connection);
	}

	for (std::map<IScript::Object*, ObjectInfo>::iterator it = localActiveObjects.begin(); it != localActiveObjects.end(); ++it) {
		(it->first)->ScriptUninitialize(*this);
	}

	for (std::set<IScript::Request::AutoWrapperBase*>::iterator p = localCallbacks.begin(); p != localCallbacks.end(); ++p) {
		delete* p;
	}

	for (std::set<IScript::BaseDelegate*>::iterator q = localReferences.begin(); q != localReferences.end(); ++q) {
		delete* q;
	}

	for (std::set<IReflectObject*>::iterator t = tempObjects.begin(); t != tempObjects.end(); ++t) {
		delete* t;
	}

	if (!manually) {
		//host.activeRequests.erase(this);
	}

	threadApi.DeleteEvent(syncCallEvent);
}

IScript* RemoteProxy::Request::GetScript() {
	return &host;
}

int RemoteProxy::Request::GetCount() {
	return (int)buffer.size() - initCount;
}

IScript::Request::TYPE RemoteProxy::Request::GetCurrentType() {
	Variant& var = buffer[buffer.size() - 1];
	assert(false); // not implemented
	return NIL;
}

static int IncreaseTableIndex(std::vector<Variant>& buffer, int count = 1) {
	Variant& var = buffer[buffer.size() - 1];
	singleton Unique intType = UniqueType<int>::Get();
	assert(var->QueryValueUnique() == intType);
	int& val = static_cast<Value<int>*>(var.Get())->value;

	if (count == 0) {
		val = 0;
	} else {
		val += count;
	}

	return val - 1;
}

template <class C>
inline void Write(RemoteProxy::Request& request, C& value) {
	std::vector<Variant>& buffer = request.buffer;
	int& tableLevel = request.tableLevel;
	int& idx = request.idx;
	String& key = request.key;

	if (tableLevel != 0) {
		Variant& arr = buffer[buffer.size() - 2];
		singleton Unique tableType = UniqueType<TableImpl>::Get();
		assert(arr.Get()->QueryValueUnique() == tableType);

		TableImpl& table = (static_cast<Value<TableImpl>*>(arr.Get()))->value;
		if (key.empty()) {
			int index = IncreaseTableIndex(buffer);
			table.arrayPart.emplace_back(Variant(value));
		} else {
			table.mapPart[key] = Variant(value);
		}

		key = "";
	} else {
		buffer.emplace_back(Variant(value));
	}
}

IScript::Request& RemoteProxy::Request::operator << (const ArrayStart& t) {
	return *this << begintable;
}

IScript::Request& RemoteProxy::Request::operator << (const TableStart&) {
	TableImpl impl;
	Variant var(impl);

	if (tableLevel != 0) {
		Variant& arr = buffer[buffer.size() - 2];
		singleton Unique tableType = UniqueType<TableImpl>::Get();
		assert(arr.Get()->QueryValueUnique() == tableType);

		TableImpl& table = (static_cast<Value<TableImpl>*>(arr.Get()))->value;
		if (key.empty()) {
			int index = IncreaseTableIndex(buffer);
			table.arrayPart.emplace_back(var);
		} else {
			table.mapPart[key] = var;
		}

		key = "";
	} else {
		buffer.emplace_back(var);
	}

	key = "";
	// load table
	buffer.emplace_back(var);
	buffer.emplace_back(Variant((int)0));
	tableLevel++;
	return *this;
}

template <class C>
inline void Read(RemoteProxy::Request& request, C& value) {
	std::vector<Variant>& buffer = request.buffer;
	int& tableLevel = request.tableLevel;
	int& idx = request.idx;
	String& key = request.key;

	if (tableLevel != 0) {
		Variant& arr = buffer[buffer.size() - 2];
		singleton Unique tableType = UniqueType<TableImpl>::Get();
		assert(arr.Get()->QueryValueUnique() == tableType);

		TableImpl& table = (static_cast<Value<TableImpl>*>(arr.Get()))->value;
		if (key.empty()) {
			int index = IncreaseTableIndex(buffer);
			Variant* var = &table.arrayPart[index];
			value = static_cast<Value<C>*>(var->Get())->value;
		} else {
			std::map<String, Variant>::iterator it = table.mapPart.find(key);
			if (it != table.mapPart.end()) {
				value = static_cast<Value<C>*>(it->second.Get())->value;
			}
		}

		key = "";
	} else {
		if (idx < (int)buffer.size()) {
			Variant* var = &buffer[idx++];

			assert(var->Get()->QueryValueUnique() == UniqueType<C>::Get());
			if (var->Get()->QueryValueUnique() == UniqueType<C>::Get()) {
				value = static_cast<Value<C>*>(var->Get())->value;
			}
		}
	}
}

IScript::Request& RemoteProxy::Request::operator >> (ArrayStart& ts) {
	TableStart t;
	*this >> t;
	ts.count = t.count;
	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (TableStart& ts) {
	if (tableLevel == 0) {
		if (idx == (int)buffer.size()) {
			// create empty table
			TableImpl impl;
			Variant var(impl);
			buffer.emplace_back(var);
		}

		Variant v = buffer[idx++];
		buffer.emplace_back(v);
	} else {
		Variant& arr = buffer[buffer.size() - 2];
		singleton Unique tableType = UniqueType<TableImpl>::Get();
		assert(arr.Get()->QueryValueUnique() == tableType);
		TableImpl& table = (static_cast<Value<TableImpl>*>(arr.Get()))->value;
		if (key.empty()) {
			int index = IncreaseTableIndex(buffer);
			buffer.emplace_back(table.arrayPart[index]);
		} else {
			std::map<String, Variant>::iterator it = table.mapPart.find(key);
			if (it != table.mapPart.end()) {
				buffer.emplace_back(it->second);
			} else {
				TableImpl impl;
				Variant var(impl);
				buffer.emplace_back(var);
			}
		}
	}

	key = "";
	buffer.emplace_back(Variant((int)0));
	Variant& v = buffer[buffer.size() - 2];
	singleton Unique tableType = UniqueType<TableImpl>::Get();
	assert(v.Get()->QueryValueUnique() == tableType);
	TableImpl& t = static_cast<Value<TableImpl>*>(v.Get())->value;
	ts.count = t.arrayPart.size();
	tableLevel++;

	return *this;
}

IScript::Request& RemoteProxy::Request::Push() {
	assert(key.empty());
	buffer.emplace_back(Variant(idx));
	buffer.emplace_back(Variant(tableLevel));
	buffer.emplace_back(Variant(initCount));
	initCount = (int)buffer.size();
	idx = initCount;
	tableLevel = 0;

	return *this;
}

IScript::Request& RemoteProxy::Request::Pop() {
	assert(key.empty());
	assert(tableLevel == 0);

	size_t org = initCount;

	initCount = (static_cast<Value<int>*>(buffer[org - 1].Get()))->value;
	tableLevel = (static_cast<Value<int>*>(buffer[org - 2].Get()))->value;
	idx = (static_cast<Value<int>*>(buffer[org - 3].Get()))->value;

	buffer.resize(org - 3);
	return *this;
}

void RemoteProxy::Request::RequestNewObject(IScript::Request& request, const String& url) {
	IScript::BaseDelegate d(host.objectCreator(url));
	request.DoLock();
	request << d;
	request.UnLock();
}

void RemoteProxy::Request::RequestQueryObject(IScript::Request& request, IScript::BaseDelegate base) {
	// fetch object
	ObjectInfo& info = base.IsNative() ? localActiveObjects[base.GetRaw()] : globalRoutines;
	request.DoLock();
	request << beginarray;

	IScript::Request::Key key;
	for (size_t i = 0; i < info.collection.size(); i++) {
		request << begintable;
		const ObjectInfo::Entry& entry = info.collection[i];
		request << key("Name") << entry.name;
		request << key("Arguments") << beginarray;
		for (size_t j = 1; j < entry.params.size(); j++) {
			request << entry.params[j].type->GetName();
		}
		request << endarray;
		request << endtable;
	}

	request << endarray;
	request.UnLock();
}

IScript::Request::Ref RemoteProxy::Request::Load(const String& script, const String& pa) {
	// OK! return pseudo remote object proxy
	if (script == "Global") {
		IScript::BaseDelegate* d = new IScript::BaseDelegate((IScript::Object*)UNIQUE_GLOBAL);
		localReferences.insert(d);

		return IScript::Request::Ref((size_t)d);
	} else {
		return IScript::Request::Ref(0);
	}
}

IScript::Request& RemoteProxy::Request::operator << (const Nil& nil) {
	assert(!isKey);
	Write(*this, nil);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const Global&) {
	assert(!isKey);
	assert(false);
	/*
	buffer.emplace_back(Variant((IDispatch*)host.globalObject));
	buffer.emplace_back(Variant(0));
	tableLevel++;*/
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const Key& k) {
	assert(key.empty());
	assert(!isKey);
	isKey = true;
	// key = k.name;
	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (Iterator& it) {
	assert(false); // not allowed
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (double value) {
	assert(!isKey);
	Write(*this, value);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (double& value) {
	assert(!isKey);
	Read(*this, value);

	return *this;
}


IScript::Request& RemoteProxy::Request::operator << (const StringView& str) {
	assert(false);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (StringView& str) {
	assert(false);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const String& str) {
	if (isKey) {
		key = str;
		isKey = false;
	} else {
		Write(*this, str);
	}

	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (String& str) {
	if (isKey) {
		key = str;
		isKey = false;
	} else {
		Read(*this, str);
	}

	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const Bytes& str) {
	assert(!isKey);
	Write(*this, str);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (Bytes& str) {
	assert(!isKey);
	Read(*this, str);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const char* str) {
	assert(str != nullptr);
	if (isKey) {
		key = str;
		isKey = false;
		return *this;
	} else {
		return *this << String(str);
	}
}

IScript::Request& RemoteProxy::Request::operator >> (const char*& str) {
	assert(false); // Not allowed
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (bool b) {
	assert(!isKey);
	Write(*this, b);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (bool& b) {
	assert(!isKey);
	Read(*this, b);
	return *this;
}

inline IScript::BaseDelegate Reverse(const IScript::BaseDelegate& value) {
	return IScript::BaseDelegate((IScript::Object*)((size_t)value.GetRaw() | (value.IsNative() ? IScript::BaseDelegate::IS_REMOTE : 0)));
}

IScript::Request& RemoteProxy::Request::operator << (const BaseDelegate& value) {
	assert(!isKey);
	IScript::BaseDelegate rev = Reverse(value);
	Write(*this, rev);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (BaseDelegate& value) {
	assert(!isKey);
	value = IScript::BaseDelegate(nullptr);
	Read(*this, value);
	// value = Reverse(value);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const AutoWrapperBase& wrapper) {
	assert(!isKey);
	assert(false); // not supported
	// const AutoWrapperBase* ptr = &wrapper;
	// Write(*this, ptr);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (int64_t u) {
	assert(!isKey);
	Write(*this, u);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (int64_t& u) {
	assert(!isKey);
	Read(*this, u);

	return *this;
}

IScript::Request& RemoteProxy::Request::MoveVariables(IScript::Request& target, size_t count) {
	assert(!isKey);
	assert(false); // not supported

	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const ArrayEnd&) {
	assert(!isKey);
	buffer.resize(buffer.size() - 2);
	tableLevel--;
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const TableEnd&) {
	assert(!isKey);
	buffer.resize(buffer.size() - 2);
	tableLevel--;
	return *this;
}

bool RemoteProxy::Request::IsValid(const BaseDelegate& d) {
	return d.GetRaw() != nullptr;
}

IScript::Request& RemoteProxy::Request::operator >> (Arguments& args) {
	assert(!isKey);
	args.count = initCount - idx + 1;
	assert(args.count > 0);

	return *this;
}

IScript::Request& RemoteProxy::Request::operator >> (Ref& ref) {
	assert(!isKey);
	BaseDelegate d;
	*this >> d;
	ref = ReferenceEx(&d);
	return *this;
}

IScript::Request& RemoteProxy::Request::operator << (const Ref& ref) {
	assert(!isKey);
	*this << *reinterpret_cast<BaseDelegate*>(ref.value);
	return *this;
}

bool RemoteProxy::Request::Call(const AutoWrapperBase& wrapper, const Request::Ref& ref) {
	assert(!isKey);
	// parse function index
	if (buffer.empty())
		return false;

	const Variant& var = buffer[initCount];
	singleton Unique int64Type = UniqueType<int64_t>::Get();
	if (!(var.Get()->QueryValueUnique() == int64Type)) {
		return false;
	}

	// pack arguments
	const Variant& callProcIdx = buffer[initCount];
	if (!(callProcIdx.Get()->QueryValueUnique() == int64Type)) {
		return false;
	}

	BaseDelegate* d = reinterpret_cast<BaseDelegate*>(ref.value);
	Packet packet;
	packet.deferred = !wrapper.IsSync();
	packet.object = d == nullptr ? 0 : (size_t)d->GetRaw();
	packet.procedure = (int64_t)(static_cast<Value<int64_t>*>(callProcIdx.Get()))->value;
	IScript::Request::AutoWrapperBase* cb = wrapper.Clone();
	packet.callback = (int64_t)cb;
	std::vector<Variant>::iterator from = buffer.begin() + initCount + 1;
#if defined(_MSC_VER) && _MSC_VER <= 1200
	std::copy(from, buffer.end(), std::back_inserter(packet.vars));
#else
	std::move(from, buffer.end(), std::back_inserter(packet.vars));
#endif
	buffer.erase(from, buffer.end());
	localCallbacks.insert(cb);
	PostPacket(packet);

	if (wrapper.IsSync()) {
		// Must wait util return, timeout or error
		// free locks
		threadApi.Wait(syncCallEvent, host.mutex);
	}

	return true;
}

IScript::Request::Ref RemoteProxy::Request::ReferenceEx(const IScript::BaseDelegate* base) {
	IScript::Object* ptr = base->GetRaw();
	if (ptr != nullptr) {
		std::map<IScript::Object*, size_t>& delta = base->IsNative() ? localObjectRefDelta : remoteObjectRefDelta;
		std::map<IScript::Object*, ObjectInfo>& info = base->IsNative() ? localActiveObjects : remoteActiveObjects;
		info[ptr].refCount++;
		delta[ptr]++;
	}

	IScript::BaseDelegate* p = new IScript::BaseDelegate(*base);
	localReferences.insert(p);
	return (size_t)p;
}

void RemoteProxy::Request::DereferenceEx(IScript::BaseDelegate* base) {
	IScript::Object* ptr = base->GetRaw();
	if (ptr != nullptr) {
		std::map<IScript::Object*, size_t>& delta = base->IsNative() ? localObjectRefDelta : remoteObjectRefDelta;
		std::map<IScript::Object*, ObjectInfo>& info = base->IsNative() ? localActiveObjects : remoteActiveObjects;
		info[ptr].refCount--;
		delta[ptr]--;
	}

	localReferences.erase(base);
	delete base;
}

IScript::Request::Ref RemoteProxy::Request::Reference(const Ref& ref) {
	return ReferenceEx(reinterpret_cast<IScript::BaseDelegate*>(ref.value));
}

IScript::Request::TYPE RemoteProxy::Request::GetReferenceType(const Ref& d) {
	return IScript::Request::OBJECT;
}

void RemoteProxy::Request::Dereference(Ref& ref) {
	DereferenceEx(reinterpret_cast<IScript::BaseDelegate*>(ref.value));
	ref.value = 0;
}

IScript::Request::AutoWrapperBase* RemoteProxy::Request::GetWrapper(const Ref& ref) {
	return nullptr;
}

const char* RemoteProxy::GetFileExt() const {
	return "rpc";
}

class ReflectRoutines : public IReflect {
public:
	// input source
	ReflectRoutines(IScript::Request& request, const IScript::BaseDelegate& d, RemoteProxy::ObjectInfo& objInfo);
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override;
	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override;

private:
	std::map<String, TWrapper<std::pair<IScript::Request::Ref, size_t>, IScript::Request&, bool>*> mapNameToWrapper;
	RemoteProxy::ObjectInfo& objectInfo;
};

ReflectRoutines::ReflectRoutines(IScript::Request& request, const IScript::BaseDelegate& d, RemoteProxy::ObjectInfo& objInfo) : IReflect(true, false), objectInfo(objInfo) {
	// read request

	if (objectInfo.needQuery) {
		IScript::Request::TableStart ns;
		request >> ns;
		objectInfo.collection.resize(ns.count);

		IScript::Request::Key key;
		for (size_t i = 0; i < ns.count; i++) {
			String name;
			request >> begintable;
			request << key("Name") >> name;
			RemoteProxy::ObjectInfo::Entry& entry = objectInfo.collection[i];
			entry.index = i;
			entry.name = name;
			entry.obj = d;
			entry.method = Wrap(&entry, &RemoteProxy::ObjectInfo::Entry::CallFilter);
			mapNameToWrapper[name] = &entry.method;

			/*
			printf("Name: %s\n", name.c_str());
			IScript::Request::TableStart ts;
			request << key("Arguments") >> ts;
			for (size_t j = 0; j < ts.count; j++) {
			request >> name;
			printf("\tArg[%d]: %s\n", (int)j, name.c_str());
			}*/
			request << endtable;
		}
		request << endtable;
		objectInfo.needQuery = false;
	} else {
		for (size_t i = 0; i < objectInfo.collection.size(); i++) {
			RemoteProxy::ObjectInfo::Entry& entry = objectInfo.collection[i];
			mapNameToWrapper[entry.name] = &entry.method;
		}
	}
}

void ReflectRoutines::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	if (s.IsBasicObject()) {
		singleton Unique wrapperType = UniqueType<IScript::MetaRemoteEntryBase>::Get();
		if (typeID->GetSize() == sizeof(TWrapper<void>)) {
			for (const MetaChainBase* p = meta; p != nullptr; p = p->GetNext()) {
				const MetaNodeBase* node = p->GetNode();
				if (!node->IsBasicObject() && node->GetUnique() == wrapperType) {
					const IScript::MetaRemoteEntryBase* wrapper = static_cast<const IScript::MetaRemoteEntryBase*>(node);
					TWrapper<void>& routineBase = *reinterpret_cast<TWrapper<void>*>(ptr);
					routineBase = wrapper->wrapper;
					TWrapper<std::pair<IScript::Request::Ref, size_t>, IScript::Request&, bool>* host = mapNameToWrapper[wrapper->name.empty() ? name : wrapper->name];
					routineBase.proxy.host = reinterpret_cast<IHost*>(host);
				}
			}
		}
	}
}

void ReflectRoutines::Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

class QueryInterfaceCallback : public IReflectObject {
public:
	QueryInterfaceCallback(const TWrapper<void, IScript::Request&, IReflectObject&, const IScript::Request::Ref&>& c, const IScript::BaseDelegate& d, IReflectObject& o, RemoteProxy::ObjectInfo& objInfo, const IScript::Request::Ref& r) : object(o), bd(d), callback(c), objectInfo(objInfo), g(r) {}

	void Invoke(IScript::Request& request) {
		RemoteProxy::Request& r = static_cast<RemoteProxy::Request&>(request);
		r.DoLock();
		r.QueryObjectInterface(objectInfo, bd, callback, object, g);
		r.UnLock();

		r.tempObjects.erase(this);
		delete this;
	}

	IReflectObject& object;
	IScript::Request::Ref g;
	IScript::BaseDelegate bd;
	TWrapper<void, IScript::Request&, IReflectObject&, const IScript::Request::Ref&> callback;
	RemoteProxy::ObjectInfo& objectInfo;
};

void RemoteProxy::Request::QueryObjectInterface(ObjectInfo& objectInfo, const IScript::BaseDelegate& d, const TWrapper<void, IScript::Request&, IReflectObject&, const Request::Ref&>& callback, IReflectObject& target, const Request::Ref& g) {
	ReflectRoutines reflect(*this, d, objectInfo);
	target(reflect);

	UnLock();
	callback(*this, target, g);
	DoLock();
}

void RemoteProxy::Request::QueryInterface(const TWrapper<void, IScript::Request&, IReflectObject&, const Request::Ref&>& callback, IReflectObject& target, const Request::Ref& g) {
	IScript::Request& request = *this;
	// Check if the object is already cached.
	IScript::BaseDelegate* d = reinterpret_cast<IScript::BaseDelegate*>(g.value);
	assert(d != nullptr);
	if (d->GetRaw() == (IScript::Object*)UNIQUE_GLOBAL) {
		QueryObjectInterface(globalRoutines, *d, callback, target, g);
	} else if (!d->IsNative()) {
		ObjectInfo& objectInfo = remoteActiveObjects[d->GetRaw()];
		if (objectInfo.needQuery) {
			QueryInterfaceCallback* cb = new QueryInterfaceCallback(callback, *d, target, objectInfo, g);
			tempObjects.insert(cb);
			IScript::BaseDelegate d((IScript::Object*)UNIQUE_GLOBAL);
			IScript::Request::Ref ref((size_t)&d);

			request.Push();
			request.Call(IScript::Request::Adapt(Wrap(cb, &QueryInterfaceCallback::Invoke)), ref, GLOBAL_INTERFACE_QUERY, g);
			request.Pop();
		} else {
			QueryObjectInterface(objectInfo, *d, callback, target, g);
		}
	}
}

void RemoteFactory::Initialize(IScript::Request& request, const TWrapper<void, IScript::Request&, IReflectObject&, const IScript::Request::Ref&>& callback) {
	globalRef = request.Load("Global", "Initialize");
	request.QueryInterface(callback, *this, globalRef);
}

TObject<IReflect>& RemoteFactory::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(NewObject)[ScriptRemoteMethod(&RemoteProxy::Request::RequestNewObject)];
		ReflectProperty(QueryObject)[ScriptRemoteMethod(&RemoteProxy::Request::RequestQueryObject)];
		ReflectProperty(globalRef);
	}

	return *this;
}

IScript* RemoteProxy::NewScript() const {
	return nullptr; // Not supported
}

bool RemoteProxy::IsClosing() const {
	assert(false);
	return false;
}

bool RemoteProxy::IsHosting() const {
	assert(false);
	return false;
}
