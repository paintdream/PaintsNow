#include "Serialization.h"
#include "../../../Core/System/MemoryStream.h"
#include "../../../Utility/SnowyStream/Resource/MeshResource.h"
#include "../../../Utility/SnowyStream/ResourceManager.h"
#include "../../../General/Driver/Filter/LZW/ZFilterLZW.h"
#include "../../../General/Driver/Filter/Json/ZFilterJson.h"

using namespace PaintsNow;

class EmptyType : public TReflected<EmptyType, IReflectObjectComplex, MetaConstructable, MetaCloneable> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
		}

		return *this;
	}

	String hi;
};

class MyType : public TReflected<MyType, IReflectObjectComplex, MetaConstructable> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(hi);
		}

		return *this;
	}

	String hi;
};

class DynMyType : public TReflected<DynMyType, SharedTiny> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(hi);
		}

		return *this;
	}

	String hi;
};

template <class X>
class MetaSimplePersist : public TReflected<MetaSimplePersist<X>, MetaStreamPersist> {
public:
	typedef TReflected<MetaSimplePersist<X>, MetaStreamPersist> BaseClass;
	virtual TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator () (reflect);
		return *this;
	}

	virtual IReflectObject* Clone() const override {
		return new MetaSimplePersist();
	}

	virtual bool Read(IStreamBase& streamBase, void* ptr) const override {
		X* object = reinterpret_cast<X*>(ptr);
		return streamBase >> *object;
	}

	virtual bool Write(IStreamBase& streamBase, const void* ptr) const override {
		const X* object = reinterpret_cast<const X*>(ptr);
		return streamBase << *object;
	}

	virtual String GetUniqueName() const override {
		return BaseClass::GetUnique()->GetBriefName();
	}

	template <class T, class D>
	inline const MetaSimplePersist& FilterField(T* t, D* d) const {
		return *this; // do nothing
	}

	template <class T, class D>
	struct RealType {
		typedef MetaSimplePersist Type;
	};

	typedef MetaSimplePersist Type;
};

template <class X>
class MetaSharedPersist : public TReflected<MetaSharedPersist<X>, MetaStreamPersist> {
public:
	typedef TReflected<MetaSharedPersist<X>, MetaStreamPersist> BaseClass;
	virtual TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator () (reflect);
		return *this;
	}

	virtual IReflectObject* Clone() const override {
		return new MetaSharedPersist();
	}

	virtual bool Read(IStreamBase& streamBase, void* ptr) const override {
		TShared<X>* object = reinterpret_cast<TShared<X>*>(ptr);
		*object = TShared<X>::From(new X());
		return streamBase >> *(*object);
	}

	virtual bool Write(IStreamBase& streamBase, const void* ptr) const override {
		const TShared<X>* object = reinterpret_cast<const TShared<X>*>(ptr);
		return streamBase << *(*object);
	}

	virtual String GetUniqueName() const override {
		return BaseClass::GetUnique()->GetBriefName();
	}

	template <class T, class D>
	inline const MetaSharedPersist& FilterField(T* t, D* d) const {
		return *this; // do nothing
	}

	template <class T, class D>
	struct RealType {
		typedef MetaSharedPersist Type;
	};

	typedef MetaSharedPersist Type;
};

class MarshallInfo : public TReflected<MarshallInfo, IReflectObjectComplex, MetaConstructable> {
public:
	MarshallInfo() : dynType(nullptr), value(0), test(0), flt(0) {}
	virtual ~MarshallInfo() {
		if (dynType != nullptr) {
			dynType->Destroy();
		}

		for (size_t i = 0; i < dynVector.size(); i++) {
			IReflectObjectComplex* c = dynVector[i];
			if (c != nullptr) {
				c->Destroy();
			}
		}
	}
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(flt)[MetaSimplePersist<float>()];
			ReflectProperty(value);
			ReflectProperty(test);
			ReflectProperty(mytype);
			ReflectProperty(empty);
			ReflectProperty(mytype2);
			ReflectProperty(mytype3)[MetaSimplePersist<IReflectObject>()];
			ReflectProperty(arr);
			ReflectProperty(arrmytype);
			ReflectProperty(arrmytypecustom)[MetaSimplePersist<IReflectObject>()];
			ReflectProperty(dynType);
			ReflectProperty(dynVector);
			ReflectProperty(fltArr)[MetaSimplePersist<float>()];
			ReflectProperty(arrstrings)[MetaSimplePersist<String>()];
			ReflectProperty(sharedTypes)[MetaSharedPersist<DynMyType>()];
		}

		return *this;
	}

	String name; // not serialized!
	int value;
	double test;
	float flt;
	MyType mytype;
	EmptyType empty;
	MyType mytype2;
	MyType mytype3;
	IReflectObjectComplex* dynType;
	std::vector<int> arr;
	std::vector<float> fltArr;
	std::vector<MyType> arrmytype;
	std::vector<MyType> arrmytypecustom;
	std::vector<String> arrstrings;
	std::vector<TShared<DynMyType> > sharedTypes;
	std::vector<IReflectObjectComplex*> dynVector;
};

class Another : public TReflected<Another, IReflectObjectComplex, MetaConstructable> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(magic);
		}

		return *this;
	}

	short magic;
};

class Derived : public TReflected<Derived, MarshallInfo> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(ch);
			ReflectProperty(extra);
		}

		return *this;
	}

	char ch;
	String extra;
};

#include "../../../Core/Driver/Filter/Pod/ZFilterPod.h"

bool Serialization::Initialize() {
	Creatable<MyType>::Init();
	return true;
}

class A : public IReflectObjectComplex {
public:
	int a;
};

class B : public IUniversalInterface {
public:
	virtual void foo() { printf("B::foo()\n"); }
	int b;
};

class C : public TReflected<C, A>, public B {
public:
	virtual void foo() { printf("C::foo()\n"); }
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		ReflectClass(Class)[ReflectInterface(BaseClass)][ReflectInterface(B)];

		return *this;
	}
	int c;
};

struct ParameterData final : public TReflected<ParameterData, IReflectObjectComplex> {
	TObject<IReflect>& operator () (IReflect& reflect) override {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(description);
		}

		return *this;
	}

	String description;
};

struct Document : public TReflected<Document, IReflectObjectComplex> {
	TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(pluginPath);
			ReflectProperty(parameters);
		}

		return *this;
	}

	String pluginPath;
	std::vector<ParameterData> parameters;
};

bool Serialization::Run(int randomSeed, int length) {
	C c;
	IReflectObject& pc = c;
	B* b = pc.QueryInterface(UniqueType<B>());
	b->foo();
	printf("Address of C = %p\n", &c);
	printf("Address of B = %p\n", b);

	MarshallInfo info;
	info.name = "HAHA";
	info.value = 123;
	info.test = 0.5;
	info.flt = 3.0f;
	info.fltArr.push_back(5.0f);
	info.mytype.hi = "Hello, world!";
	info.mytype2.hi = "ALOHA";
	info.mytype3.hi = "Custom serialization";
	info.arr.emplace_back(1);
	info.arr.emplace_back(3);
	MyType s;
	s.hi = "In Container";
	EmptyType e;
	e.hi = "Never serialized.";

	MyType t;
	t.hi = "ArrString";
	info.arrmytype.emplace_back(t);
	info.arrmytype.emplace_back(t);
	info.arrmytypecustom.emplace_back(t);
	t.hi = "CustomType";
	info.arrmytypecustom.emplace_back(t);
	info.arrstrings.emplace_back("Sample String");
	info.arrstrings.emplace_back("Another String");

	TShared<DynMyType> m = TShared<DynMyType>::From(new DynMyType());
	m->hi = "Shared";
	info.sharedTypes.emplace_back(m);
	TShared<DynMyType> n = TShared<DynMyType>::From(new DynMyType());
	n->hi = "Another Shared";
	info.sharedTypes.emplace_back(n);

	MyType* d = new MyType();
	d->hi = "Dynamic!!";
	info.dynType = d;
	MyType* vec = new MyType();
	vec->hi = "Vectorized~!";
	info.dynVector.emplace_back(vec);

	Derived derived;
	derived.name = "Derived";
	derived.value = 456;
	derived.test = 0.3;
	derived.mytype.hi = "MyTYPE";
	derived.mytype2.hi = "HI2";
	derived.ch = 'a';
	derived.extra = "Extra";

	MemoryStream stream(0x1000);
	ZFilterPod pod;

	Document doc;
	doc.pluginPath = "../script/datahunter.lua";
	ParameterData param;
	param.description = "desc";
	doc.parameters.push_back(std::move(param));
	IStreamBase* ft = pod.CreateFilter(stream);
	*ft << doc;
	ft->Destroy();
	stream.Seek(IStreamBase::BEGIN, 0);
	ft = pod.CreateFilter(stream);
	Document wdoc;
	*ft >> wdoc;
	ft->Destroy();


#define USE_FILTER

#ifdef USE_FILTER
	IStreamBase* filter = pod.CreateFilter(stream);
	IStreamBase& finalStream = *filter;
#else
	IStreamBase& finalStream = stream;
#endif
	finalStream << s;
	finalStream << e;
	finalStream << info;
	finalStream << derived;

#ifdef USE_FILTER
	filter->Destroy();
#endif

	MarshallInfo target;
	Derived targetDerived;
	String str;
	stream.Seek(IStreamBase::BEGIN, 0);

#ifdef USE_FILTER
	IStreamBase* filterAgain = pod.CreateFilter(stream);
	IStreamBase& finalStreamAgain = *filterAgain;
#else
	IStreamBase& finalStreamAgain = stream;
#endif
	finalStreamAgain >> s;
	e.hi = "changed but not deserialized";
	finalStreamAgain >> e;
	finalStreamAgain >> target;
	finalStreamAgain >> targetDerived;

#ifdef USE_FILTER
	filterAgain->Destroy();
#endif

	ZFilterLZW lzw;
	MemoryStream ms(0x1000);
	IStreamBase* f = lzw.CreateFilter(ms);
	const char* data = "asdfsdfoaiwhsdfasdjfgha;lsddlahfkscfdhasfsldkfhasbjfabababa";
	size_t len = strlen(data);
	size_t l = len;
	f->Write(data, l);
	f->Destroy();
	ms.Seek(IStreamBase::BEGIN, 0);
	f = lzw.CreateFilter(ms);
	char result[1024] = { 0 };
	f->Read(result, len);
	printf("Recover data: %s\n", data);
	printf("Recover data: %s\n", result);
	f->Destroy();

	ZFilterJson json;
	MemoryStream jsonStream(0x1000);
	IStreamBase* j = json.CreateFilter(jsonStream);
	*j << info;
	j->Destroy();

	jsonStream.Seek(IStreamBase::BEGIN, 0);
	IStreamBase* jr = json.CreateFilter(jsonStream);
	MarshallInfo jsonReadback;
	*jr >> jsonReadback;
	jr->Destroy();
	return true;
}

void Serialization::Summary() {

}

TObject<IReflect>& Serialization::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}