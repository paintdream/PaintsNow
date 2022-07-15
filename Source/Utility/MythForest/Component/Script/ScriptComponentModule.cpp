#include "ScriptComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

class Reflector : public IReflect {
public:
	Reflector(std::unordered_map<String, size_t>& m) : mapEventNameToID(m), IReflect(false, false, false, true) {}

	void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) override {
		mapEventNameToID[name] = value;
	}

private:
	std::unordered_map<String, size_t>& mapEventNameToID;
};

CREATE_MODULE(ScriptComponentModule);
ScriptComponentModule::ScriptComponentModule(Engine& engine) : BaseClass(engine) {
	// register enums
	Reflector reflector(mapEventNameToID);
	(*engine.GetComponentModuleFromName("EventComponent"))(reflector);
}

ScriptComponentModule::~ScriptComponentModule() {}

TObject<IReflect>& ScriptComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestSetHandler)[ScriptMethodLocked = "SetHandler"];
	}

	return *this;
}

TShared<ScriptComponent> ScriptComponentModule::RequestNew(IScript::Request& request, const String& name) {
	CHECK_REFERENCES_NONE();
	TShared<ScriptComponent> scriptComponent = TShared<ScriptComponent>::From(allocator->New(std::ref(name)));

	return scriptComponent;
}

void ScriptComponentModule::RequestSetHandler(IScript::Request& request, IScript::Delegate<ScriptComponent> scriptComponent, const String& event, IScript::Request::Ref handler) {
	if (handler) {
		CHECK_REFERENCES_WITH_TYPE_LOCKED(handler, IScript::Request::FUNCTION);
	}

	std::unordered_map<String, size_t>::iterator it = mapEventNameToID.find(event);
	if (it == mapEventNameToID.end()) {
		if (handler) {
			request.Dereference(handler);
		}

		request.Error(String("Unable to find event: ") + event);
	} else {
		scriptComponent->SetHandler(request, static_cast<Event::EVENT_ID>((*it).second), handler);
	}
}
