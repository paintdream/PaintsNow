#include "RPC.h"
#include "../../../General/Driver/Network/LibEvent/ZNetworkLibEvent.h"
#include "../../../Core/Driver/Thread/Pthread/ZThreadPthread.h"
#include "../../../General/Misc/RemoteProxy.h"

using namespace PaintsNow;

class Foo : public TReflected<Foo, IScript::Library> {
public:
	Foo() {}

	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectMethod()) {
			ReflectMethod(RequestNewFoo)[ScriptMethod = "NewFoo"];
		}

		return *this;
	}

	void RequestNewFoo(IScript::Request& request, int a, const String& str) {

	}
};

class PrefabFactory : public TWrapper<IScript::Object*, const String&> {
public:
	PrefabFactory(IThread& threadApi) : TWrapper<IScript::Object*, const String&>(Wrap(this, &PrefabFactory::CreateObject)), thread(threadApi) {}
	class Prefab : public TReflected<Prefab, IScript::Object> {
	public:
		virtual TObject<IReflect>& operator () (IReflect& reflect) {
			BaseClass::operator () (reflect);
			if (reflect.IsReflectMethod()) {
				ReflectMethod(Test)[ScriptMethod];
				ReflectMethod(Complete)[ScriptMethod];
			}

			return *this;
		}

		void Test(IScript::Request& request, uint32_t p, const String& message, int value) {
			printf("Message: %s : %d, %d\n", message.c_str(), p, value);
		}

		void Complete(IScript::Request& request, const String& message) {
			printf("Complete: %s\n", message.c_str());
		}

		virtual void ScriptUninitialize(IScript::Request& request) {}
	};

	IScript::Object* CreateObject(const String& entry) const {
		printf("Hello %s \n", entry.c_str());
		return new Prefab();
	}

	IThread& thread;
};

void Then(IScript::Request& request) {
	IScript::Request::ArrayStart ns;
	request >> ns;
	for (size_t i = 0; i < ns.count; i++) {
		String name;
		request >> begintable;
		request << key("Name") >> name;
		printf("Name: %s\n", name.c_str());
		IScript::Request::ArrayStart ts;
		request << key("Arguments") >> ts;
		for (size_t j = 0; j < ts.count; j++) {
			request >> name;
			printf("\tArg[%d]: %s\n", (int)j, name.c_str());
		}
		request << endarray;
		request << endtable;
	}
	request << endarray;
}

void After(IScript::Request& request, IScript::Request::Ref prefab) {
	request.DoLock();
	request.Push();
	request.Call(prefab, 0, String("abcasdfwhaodkjfo;asld;asdgihaosdfi"));
	request.Pop();
	request.UnLock();
}

struct MyPrefab : public TReflected<MyPrefab, IReflectObjectComplex> {
public:
	virtual TObject<IReflect>& operator () (IReflect& reflect) {
		BaseClass::operator () (reflect);
		if (reflect.IsReflectProperty()) {
			ReflectProperty(Test)[ScriptRemoteMethod(&PrefabFactory::Prefab::Test)];
			ReflectProperty(Complete)[ScriptRemoteMethod(&PrefabFactory::Prefab::Complete)];
		}
		return *this;
	}
	TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, uint32_t, const String&, int> Test;
	TWrapper<void, const IScript::Request::AutoWrapperBase&, IScript::Request&, const String&> Complete;
} myPrefab;

void OnFinish(IScript::Request& request) {
	printf("FINISH!\n");
}

void OnQueryPrefab(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref) {
	assert(&myPrefab == &inter);
	String s = "Hello, World";

	request.DoLock();
	request.Push();
	myPrefab.Test(request.Adapt(Wrap(OnFinish)), request, 666, s, 1234);
	request.Pop();
	request.Push();
	myPrefab.Complete(request.Adapt(Wrap(OnFinish)), request, s);
	request.Pop();
	request.UnLock();

	/*
	request.DoLock();
	request.Push();
	request.Call(ref, 0, "abcasdfwhaodkjfo;asld;asdgihaosdfi");
	request.Pop();
	request.UnLock();*/
}

void OnQuery(IScript::Request& request, IReflectObject& inter, const IScript::Request::Ref& ref) {
	RemoteFactory& testInterface = static_cast<RemoteFactory&>(inter);
	IScript::Request::Ref prefab;
	request.DoLock();
	request.Push();

	// You can choose either sync or async call.
	testInterface.NewObject(IScript::Request::Sync(), request, "Prefab");
	request >> prefab;

	request.QueryInterface(Wrap(OnQueryPrefab), myPrefab, prefab);
	request.Pop();
	request.UnLock();

	/*
	if (prefab) {
	request.Push();
	request.Call(prefab, 0, "abcasdfwhaodkjfo;asld;asdgihaosdfi");
	request.Pop();
	}*/
}

void OnConnect(IScript::Request& request, bool isServer, RemoteProxy::STATUS status, const String& info) {
	// sync call
	printf("Connecting ... %d, %d\n", isServer, status);
}

void TestRPC() {
}

bool RPC::Initialize() {
	return true;
}

bool RPC::Run(int randomSeed, int length) {
	ZThreadPthread uniqueThreadApi;
	Foo foo;
	IScript::Library* module = foo.QueryInterface(UniqueType<IScript::Library>());
	IScript::Object* object = foo.QueryInterface(UniqueType<IScript::Object>());

	ZNetworkLibEvent network(uniqueThreadApi);
	ITunnel& tunnel = network;

	const PrefabFactory factory(uniqueThreadApi);

	RemoteProxy serverProxy(uniqueThreadApi, tunnel, factory, "127.0.0.1:16384", Wrap(OnConnect));
	serverProxy.Run();

	for (int i = 0; i < length; i++) {
		RemoteProxy clientProxy(uniqueThreadApi, tunnel, factory, "127.0.0.1:16385");
		clientProxy.Run();
		// async create request
		IScript::Request* r = clientProxy.NewRequest("127.0.0.1:16384");
		if (r == nullptr) {
			printf("Unable to connect RPC server. Stop.\n");
			break;
		}

		r->Destroy();
		r = clientProxy.NewRequest("127.0.0.1:16384");

		IScript::Request& request = *r;
		request.DoLock();
		request.Push();
		IScript::Request::Ref global = request.Load("Global", "Initialize");
		RemoteFactory testInterface;
		request.QueryInterface(Wrap(OnQuery), testInterface, global);
		// parse result
		request.Pop();
		request.UnLock();
		getchar();

		// Close connection
		r->Destroy();
	}

	return true;
}

void RPC::Summary() {}

TObject<IReflect>& RPC::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}