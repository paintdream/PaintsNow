#include "Annotation.h"

using namespace PaintsNow;

class Inner : public TReflected<Inner, IReflectObjectComplex> {
public:
	Inner() {}
	virtual ~Inner() {}

	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(info);
			ReflectProperty(value);
			ReflectProperty(db);
		}

		return *this;
	}

	String info;
	int value;
	double db;
};

class InsightMe : public TReflected<InsightMe, IReflectObjectComplex> {
public:
	InsightMe(int x = 0, char h = 0, char p = 0) : a(x), c(h) {
	}

	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(a);
			ReflectProperty(c)[Note = "Test"];
			ReflectProperty(text);
			ReflectProperty(inner);
			ReflectProperty(vecValues);
			ReflectProperty(vecInner);
		}

		if (reflect.IsReflectMethod()) {
			ReflectMethod(func)[Note = "Hello"][Note = "Abc"];
		}

		return *this;
	}

	void func(int a, char& b, short* c, uint32_t, double d) {}

	int a;
	char c;
	String text;
	Inner inner;
	std::vector<int> vecValues;
	std::vector<Inner> vecInner;
};

class OldInsightMe : public TReflected<OldInsightMe, IReflectObjectComplex> {
public:
	OldInsightMe(int x = 0, char h = 0, char p = 0) : a(x), c(h) {
	}

	TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(a);
			ReflectProperty(c)[Note = "Test"];
			ReflectProperty(inner);

			ReflectProperty(vecValues);
			ReflectProperty(vecInner);
		}

		if (reflect.IsReflectMethod()) {
			ReflectMethod(func)[Note = "Hello"][Note = "Abc"];
		}

		return *this;
	}

	void func(int a, char& b, short* c, uint32_t, double d) {}
private:
	int a;
	char c;
	Inner inner;

	std::vector<int> vecValues;
	std::vector<Inner> vecInner;
};

void TestFunc(int a, String b) {
	//	int d = 0;
}

int TestFunc2(char a, float b) {
	//	int c = 0;
	return 0;
}

class Clos {
public:
	Clos() : state("hahahahah") {}
	~Clos() {
		printf("State: %s\n", state.c_str());

	}
	void TestFunc3(int c, const String& d) {
		printf("State: %s\n", state.c_str());
	}

	String state;
};

static void Test() {
#if !defined(_MSC_VER) || _MSC_VER > 1200
	TWrapper<int, int> p = WrapClosure([](int a)->int {
		printf("%d\n", a);
		return a;
	});

	int m = p(1);
	TWrapper<int, int> k = WrapClosure([m](int a)->int {
		printf("%d\n", a + m);
		return a - m;
	});

	k(2);
#endif

	TWrapper<void, int, String> func = TestFunc;
	TWrapper<int, char, float> func2 = TestFunc2;
	Clos* cl = new Clos();
	TWrapper<void, int, const String&> cc = WrapClosure(std::move(*cl), &Clos::TestFunc3);
	delete cl;
	cc(2, "haha");

	InsightMe me(1, 1, 1);
	me.vecValues.emplace_back(1314);
	Inner inner;
	inner.info = "test";
	me.vecInner.emplace_back(inner);
	me.vecValues.emplace_back(333);
	me.inner.value = 54321;
	me.inner.db = 123.0;
	me.inner.info = "hello";
	Creatable<Inner>();
	Creatable<OldInsightMe>();
	Creatable<InsightMe>();
	OldInsightMe old(2, 2, 2);

	func(1, "2");
	func2('a', 0.1f);
}

bool Annotation::Initialize() {
	return true;
}

bool Annotation::Run(int randomSeed, int length) {
	Test();
	return true;
}

void Annotation::Summary() {}

TObject<IReflect>& Annotation::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}