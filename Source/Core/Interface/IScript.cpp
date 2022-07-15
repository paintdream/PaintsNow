#include "IScript.h"
#include "../Template/TBuffer.h"

namespace PaintsNow {
	IScript::MetaLibrary ScriptLibrary;
	IScript::MetaMethod ScriptMethod("", false);
	IScript::MetaMethod ScriptMethodLocked("", true);
	IScript::MetaVariable ScriptVariable;
	IScript::Request::TableStart begintable;
	IScript::Request::TableEnd endtable;
	IScript::Request::ArrayStart beginarray;
	IScript::Request::ArrayEnd endarray;
	IScript::Request::Key key;
	IScript::Request::Nil nil;
	IScript::Request::Global global;
	IScript::Request::Sync sync;
}

using namespace PaintsNow;

IScript::Object::Object() {}

IScript::Object::~Object() {
	// printf("OBJECT DELETED %p\n", this);
}

void IScript::Object::ScriptUninitialize(Request& request) {
	Destroy();
}

void IScript::Object::ScriptInitialize(Request& request) {}

IScript::Library::Library() {}

IScript::Library::~Library() {}

void IScript::Library::TickDevice(IDevice& device) {}

void IScript::Request::QueryInterface(const TWrapper<void, IScript::Request&, IReflectObject&, const Request::Ref&>& callback, IReflectObject& target, const Request::Ref& g) {
	// exports no interfaces by default
	callback(*this, target, g);
}

IScript::MetaLibrary::MetaLibrary(const String& n) : name(n) {}

TObject<IReflect>& IScript::MetaLibrary::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(name);
	}

	return *this;
}

IScript::MetaLibrary IScript::MetaLibrary::operator = (const String& value) {
	return MetaLibrary(value);
}

template <bool init>
class Boostrapper : public IReflect {
public:
	Boostrapper() : IReflect(true, false) {}
	~Boostrapper() override {
		if (!init) {
			for (size_t i = libraries.size(); i > 0; i--) {
				libraries[i - 1]->Uninitialize();
			}
		}
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaLibrary>::Get();
		
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<IScript::Library>()) != nullptr) {
			for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
				const MetaNodeBase* node = t->GetNode();
				if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
					IScript::Library& lib = static_cast<IScript::Library&>(s);
					if (init) {
						lib.Initialize();
					} else {
						libraries.emplace_back(&lib);
					}
				}
			}
		}
	}

	std::vector<IScript::Library*> libraries;
};

void IScript::Library::Initialize() {
	Boostrapper<true> bootstrapper;
	(*this)(bootstrapper);
}

void IScript::Library::Uninitialize() {
	Boostrapper<false> bootstrapper;
	(*this)(bootstrapper);
}

class Registar : public IReflect {
public:
	Registar(IScript::Request& req) : IReflect(true, true), request(req) {}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaLibrary>::Get();
		
		if (!s.IsBasicObject() && s.QueryInterface(UniqueType<IScript::Library>()) != nullptr) {
			for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
				const MetaNodeBase* node = t->GetNode();
				if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
					const IScript::MetaLibrary* entry = static_cast<const IScript::MetaLibrary*>(node);
					String n = entry->name.empty() ? name : entry->name;
					IScript::Library& lib = static_cast<IScript::Library&>(s);
					request << key(n) << lib;
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaMethod::TypedBase>::Get();
		for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
			const MetaNodeBase* node = t->GetNode();
			if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
				const IScript::MetaMethod::TypedBase* entry = static_cast<const IScript::MetaMethod::TypedBase*>(node);
				entry->Register(request, name);
			}
		}
	}

	IScript::Request& request;
};

void IScript::Library::Register(IScript::Request& request) {
	Registar reg(request);
	(*this)(reg);
}

void IScript::Library::ScriptRequire(IScript::Request& request) {
	request << begintable;
	request << key("__delegate__") << this; // holding this.
	Register(request);
	request << endtable;
}

void IScript::Library::ScriptInitialize(IScript::Request& request) {
}

void IScript::Library::ScriptUninitialize(IScript::Request& request) {
}

IScript::MetaVariable::MetaVariable(const String& k) : name(k) {}
IScript::MetaVariable::~MetaVariable() {}
IScript::MetaVariable IScript::MetaVariable::operator = (const String& k) {
	return MetaVariable(k);
}

template <bool read>
class Serializer : public IReflect {
public:
	Serializer(IScript::Request& req) : IReflect(true, false), request(req) {}
	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaVariable::TypedBase>::Get();
		for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
			const MetaNodeBase* node = t->GetNode();
			if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
				IScript::MetaVariable::TypedBase* entry = const_cast<IScript::MetaVariable::TypedBase*>(static_cast<const IScript::MetaVariable::TypedBase*>(node));

				if (s.IsIterator()) {
					IScript::Request::TableStart ts;
					ts.count = 0;
					IIterator& it = static_cast<IIterator&>(s);

					if (read) {
						request << key(entry->name.empty() ? name : entry->name.c_str()) >> ts;
						it.Initialize((size_t)ts.count);
					} else {
						request << key(entry->name.empty() ? name : entry->name.c_str()) << begintable;
					}

					if (it.IsElementBasicObject()) {
						while (it.Next()) {
							void* ptr = it.Get();
							if (read) {
								entry->Read(request, false, name, ptr);
							} else {
								entry->Write(request, false, name, ptr);
							}
						}
					} else {
						while (it.Next()) {
							(*reinterpret_cast<IReflectObject*>(it.Get()))(*this);
						}
					}

					if (read) {
						request << endtable;
					} else {
						request << endtable;
					}
				} else {
					if (read) {
						entry->Read(request, true, name, nullptr);
					} else {
						entry->Write(request, true, name, nullptr);
					}
				}
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	IScript::Request& request;
};

IScript::IScript(IThread& api) : ISyncObject(api) {}

IScript::~IScript() {
}

IScript::Request::Request() : requestPool(nullptr), next(nullptr) {}

void IScript::Request::SetRequestPool(IScript::RequestPool* p) {
	if (p != nullptr) {
		assert(GetScript() == &p->GetScript());
	}

	requestPool = p;
}

IScript::RequestPool* IScript::Request::GetRequestPool() {
	return requestPool;
}

void* IScript::Request::GetNativeScript() {
	return nullptr;
}

IScript::Request::~Request() {}

void IScript::Request::DoLock() {
	GetScript()->DoLock();
}

void IScript::Request::UnLock() {
	GetScript()->UnLock();
}

bool IScript::Request::TryLock() {
	return GetScript()->TryLock();
}

#ifdef _MSC_VER
IScript::Request& IScript::Request::operator >> (long& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<long>(v);
	return *this;
}

IScript::Request& IScript::Request::operator >> (unsigned long& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<unsigned long>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (long t) {
	(*this) << (int64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator << (unsigned long t) {
	(*this) << (int64_t)t;
	return *this;
}
#endif

IScript::Request& IScript::Request::operator >> (int8_t& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<int8_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (int8_t t) {
	(*this) << (int64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (int16_t& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<int16_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (int16_t t) {
	(*this) << (int64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (int32_t& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<int32_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (int32_t t) {
	(*this) << (int64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (uint8_t& t) {
	uint64_t v = 0;
	(*this) >> v;
	t = verify_cast<uint8_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (uint8_t t) {
	(*this) << (uint64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (uint16_t& t) {
	uint64_t v = 0;
	(*this) >> v;
	t = verify_cast<uint16_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (uint16_t t) {
	(*this) << (uint64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (uint32_t& t) {
	uint64_t v = 0;
	(*this) >> v;
	t = verify_cast<uint32_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (uint32_t t) {
	(*this) << (uint64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator >> (uint64_t& t) {
	int64_t v = 0;
	(*this) >> v;
	t = verify_cast<uint64_t>(v);
	return *this;
}

IScript::Request& IScript::Request::operator << (uint64_t t) {
	(*this) << (int64_t)t;
	return *this;
}

IScript::Request& IScript::Request::operator << (Library& module) {
	module.ScriptRequire(*this);
	return *this;
}

IScript::Request& IScript::Request::operator >> (Void&) {
	return *this;
}

IScript::Request& IScript::Request::operator << (const Void&) {
	return *this;
}

IScript::Request& IScript::Request::operator << (float value) {
	*this << (double)value;
	return *this;
}

IScript::Request& IScript::Request::operator >> (float& value) {
	double db = 0;
	*this >> db;
	value = (float)db;
	return *this;
}

IScript::Request& IScript::Request::operator >> (IReflectObjectComplex& object) {
	assert(!object.IsBasicObject());
	*this >> begintable;
	Serializer<true> s(*this);
	object(s);
	*this << endtable;

	return *this;
}

IScript::Request& IScript::Request::operator << (const IReflectObjectComplex& object) {
	assert(!object.IsBasicObject());
	*this << begintable;
	Serializer<false> s(*this);
	(const_cast<IReflectObjectComplex&>(object))(s);
	*this << endtable;

	return *this;
}

IScript::Request& IScript::Request::operator << (Unique unique) {
	return *this << unique->GetName();
}

IScript::Request& IScript::Request::operator >> (Unique& unique) {
	String str;
	*this >> str;
	unique = Unique(str);
	return *this;
}

bool IScript::IsTypeCompatible(Request::TYPE target, Request::TYPE source) const {
	return target == source;
}

void IScript::SetErrorHandler(const TWrapper<void, Request&, const String&>& handler) {
	errorHandler = handler;
}

void IScript::SetDispatcher(const TWrapper<void, Request&, IHost*, size_t, const TWrapper<void, Request&>&>& disp) {
	dispatcher = disp;
}

const TWrapper<void, IScript::Request&, IHost*, size_t, const TWrapper<void, IScript::Request&>&>& IScript::GetDispatcher() const {
	return dispatcher;
}

void IScript::Request::Error(const String& msg) {
	const TWrapper<void, Request&, const String&>& handler = GetScript()->errorHandler;
	if (handler) {
		handler(*this, msg);
	}
}

/*
#include <Windows.h>

void IScript::DoLock() {
	DWORD id = ::GetCurrentThreadId();

	printf("Thread ID %d try to lock script\n", id);
	ISyncObject::DoLock();
	printf("Thread ID %d sucessfully locked script\n", id);
}

void IScript::UnLock() {
	DWORD id = ::GetCurrentThreadId();
	printf("Thread ID %d unlocked script\n", id);
	ISyncObject::UnLock();
}*/

void IScript::Reset() {
}

bool IScript::Request::AutoWrapperBase::IsSync() const {
	return false;
}

bool IScript::Request::Sync::IsSync() const {
	return true;
}

void IScript::Request::Sync::Execute(Request& request) const {
	assert(false);
}

IScript::Request::AutoWrapperBase* IScript::Request::Sync::Clone() const {
	return nullptr;
}

IScript::MetaMethod::MetaMethod(const String& k, bool lock) : key(k), lockOnCall(lock) {}
IScript::MetaMethod::~MetaMethod() {}

IScript::MetaMethod IScript::MetaMethod::operator = (const String& key) {
	return MetaMethod(key, lockOnCall);
}

IScript::RequestPool::RequestPool(IScript& pscript, uint32_t psize) : requestPool(*this, psize), script(pscript) {}

IScript::Request* IScript::RequestPool::allocate(size_t n) {
	assert(n == 1);
	assert(!script.IsResetting());
	script.DoLock();
	IScript::Request* request = script.NewRequest();
	request->SetRequestPool(this);
	script.UnLock();

	return request;
}

void IScript::RequestPool::construct(IScript::Request* request) {}
void IScript::RequestPool::destroy(IScript::Request* request) {}

void IScript::RequestPool::deallocate(IScript::Request* request, size_t n) {
	assert(n == 1);
	assert(!script.IsResetting());
	script.DoLock();
	assert(request->GetRequestPool() == this);
	request->Destroy();
	script.UnLock();
}

IScript& IScript::RequestPool::GetScript() {
	return script;
}
