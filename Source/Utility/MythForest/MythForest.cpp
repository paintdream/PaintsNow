#include "MythForest.h"
#include "Component/Event/EventComponentModule.h"
#define USE_FRAME_CAPTURE (!CMAKE_PAINTSNOW || ADD_DEBUGGER_RENDERDOC_BUILTIN)

using namespace PaintsNow;

extern Module* CreateAnimationComponentModule(Engine&);
extern Module* CreateBatchComponentModule(Engine&);
extern Module* CreateBridgeComponentModule(Engine&);
extern Module* CreateCacheComponentModule(Engine&);
extern Module* CreateCameraComponentModule(Engine&);
extern Module* CreateComputeComponentModule(Engine&);
extern Module* CreateDataComponentModule(Engine&);
extern Module* CreateEnvCubeComponentModule(Engine&);
extern Module* CreateEventComponentModule(Engine&);
extern Module* CreateEventGraphComponentModule(Engine&);
extern Module* CreateExplorerComponentModule(Engine&);
extern Module* CreateFieldComponentModule(Engine&);
extern Module* CreateFollowComponentModule(Engine&);
extern Module* CreateFormComponentModule(Engine&);
extern Module* CreateLayoutComponentModule(Engine&);
extern Module* CreateLightComponentModule(Engine&);
extern Module* CreateModelComponentModule(Engine&);
extern Module* CreateNavigateComponentModule(Engine&);
extern Module* CreateParticleComponentModule(Engine&);
extern Module* CreatePhaseComponentModule(Engine&);
extern Module* CreateProfileComponentModule(Engine&);
extern Module* CreateRasterizeComponentModule(Engine&);
extern Module* CreateRayTraceComponentModule(Engine&);
extern Module* CreateRenderFlowComponentModule(Engine&);
extern Module* CreateScriptComponentModule(Engine&);
extern Module* CreateShaderComponentModule(Engine&);
extern Module* CreateShapeComponentModule(Engine&);
extern Module* CreateSkyComponentModule(Engine&);
extern Module* CreateSoundComponentModule(Engine&);
extern Module* CreateSpaceComponentModule(Engine&);
extern Module* CreateStreamComponentModule(Engine&);
extern Module* CreateSurfaceComponentModule(Engine&);
extern Module* CreateTapeComponentModule(Engine&);
extern Module* CreateTerrainComponentModule(Engine&);
extern Module* CreateTextViewComponentModule(Engine&);
extern Module* CreateTransformComponentModule(Engine&);
extern Module* CreateVisibilityComponentModule(Engine&);
extern Module* CreateWidgetComponentModule(Engine&);

#if USE_FRAME_CAPTURE
#include "../../General/Driver/Debugger/RenderDoc/ZDebuggerRenderDoc.h"
static ZDebuggerRenderDoc debugger;
#endif

MythForest::MythForest(Interfaces& interfaces, SnowyStream& snowyStream, BridgeSunset& bridgeSunset)
	: engine(interfaces, bridgeSunset, snowyStream) {
	entityAllocator = TShared<Entity::Allocator>::From(new Entity::Allocator());
}

MythForest::~MythForest() {}

void MythForest::Initialize() {
	// add builtin modules
	engine.InstallModule(CreateAnimationComponentModule(engine));
	engine.InstallModule(CreateBatchComponentModule(engine));
	engine.InstallModule(CreateBridgeComponentModule(engine));
	engine.InstallModule(CreateCacheComponentModule(engine));
	engine.InstallModule(CreateCameraComponentModule(engine));
	engine.InstallModule(CreateComputeComponentModule(engine));
	engine.InstallModule(CreateDataComponentModule(engine));
	engine.InstallModule(CreateEnvCubeComponentModule(engine));
	engine.InstallModule(CreateEventComponentModule(engine));
	engine.InstallModule(CreateEventGraphComponentModule(engine));
	engine.InstallModule(CreateExplorerComponentModule(engine));
	engine.InstallModule(CreateFieldComponentModule(engine));
	engine.InstallModule(CreateFollowComponentModule(engine));
	engine.InstallModule(CreateFormComponentModule(engine));
	engine.InstallModule(CreateLayoutComponentModule(engine));
	engine.InstallModule(CreateLightComponentModule(engine));
	engine.InstallModule(CreateModelComponentModule(engine));
	engine.InstallModule(CreateNavigateComponentModule(engine));
	engine.InstallModule(CreateParticleComponentModule(engine));
	engine.InstallModule(CreatePhaseComponentModule(engine));
	engine.InstallModule(CreateProfileComponentModule(engine));
	engine.InstallModule(CreateRasterizeComponentModule(engine));
	engine.InstallModule(CreateRayTraceComponentModule(engine));
	engine.InstallModule(CreateRenderFlowComponentModule(engine));
	engine.InstallModule(CreateScriptComponentModule(engine));
	engine.InstallModule(CreateShaderComponentModule(engine));
	engine.InstallModule(CreateShapeComponentModule(engine));
	engine.InstallModule(CreateSkyComponentModule(engine));
	engine.InstallModule(CreateSoundComponentModule(engine));
	engine.InstallModule(CreateSpaceComponentModule(engine));
	engine.InstallModule(CreateStreamComponentModule(engine));
	engine.InstallModule(CreateSurfaceComponentModule(engine));
	engine.InstallModule(CreateTapeComponentModule(engine));
	engine.InstallModule(CreateTerrainComponentModule(engine));
	engine.InstallModule(CreateTextViewComponentModule(engine));
	engine.InstallModule(CreateTransformComponentModule(engine));
	engine.InstallModule(CreateVisibilityComponentModule(engine));
	engine.InstallModule(CreateWidgetComponentModule(engine));
}

void MythForest::ScriptUninitialize(IScript::Request& request) {
	while (!nextFrameListeners.Empty()) {
		request.Dereference(nextFrameListeners.Top().second);
		nextFrameListeners.Pop();
	}

	BaseClass::ScriptInitialize(request);
}

void MythForest::Uninitialize() {
	engine.Clear();
}

Engine& MythForest::GetEngine() {
	return engine;
}

TShared<Entity> MythForest::CreateEntity(int32_t warp) {
	TShared<Entity> entity = TShared<Entity>::From(entityAllocator->New(std::ref(engine)));
	entity->SetWarpIndex(warp < 0 ? engine.GetKernel().GetCurrentWarpIndex() : (uint32_t)warp);
	return entity;
}

TObject<IReflect>& MythForest::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(engine)[Runtime];

		std::unordered_map<String, Module*>& moduleMap = engine.GetModuleMap();
		for (std::unordered_map<String, Module*>::iterator it = moduleMap.begin(); it != moduleMap.end(); ++it) {
			*CreatePropertyWriter(reflect, this, *(*it).second, (*it).first.c_str())[ScriptLibrary = (*it).first + "Module"];
		}
	}

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestEnumerateComponentModules)[ScriptMethodLocked = "EnumerateComponentModules"];
		ReflectMethod(RequestNewEntity)[ScriptMethodLocked = "NewEntity"];
		ReflectMethod(RequestUpdateEntity)[ScriptMethod = "UpdateEntity"];
		ReflectMethod(RequestAddEntityComponent)[ScriptMethodLocked = "AddEntityComponent"];
		ReflectMethod(RequestRemoveEntityComponent)[ScriptMethodLocked = "RemoveEntityComponent"];
		ReflectMethod(RequestGetUniqueEntityComponent)[ScriptMethodLocked = "GetUniqueEntityComponent"];
		ReflectMethod(RequestGetEntityComponents)[ScriptMethodLocked = "GetEntityComponents"];
		ReflectMethod(RequestGetComponentType)[ScriptMethodLocked = "GetComponentType"];
		ReflectMethod(RequestClearEntityComponents)[ScriptMethod = "ClearEntityComponents"];
		ReflectMethod(RequestGetFrameTickDelta)[ScriptMethodLocked = "GetFrameTickDelta"];
		ReflectMethod(RequestGetEntityBoundingBox)[ScriptMethodLocked = "GetEntityBoundingBox"];
		ReflectMethod(RequestWaitForNextFrame)[ScriptMethod = "WaitForNextFrame"];
		ReflectMethod(RequestRaycast)[ScriptMethod = "Raycast"];
		ReflectMethod(RequestCaptureFrame)[ScriptMethod = "CaptureFrame"];
		ReflectMethod(RequestPostEvent)[ScriptMethod = "PostEvent"];
	}

	return *this;
}

void MythForest::TickDevice(IDevice& device) {
	if (&device == &engine.interfaces.render) {
		std::vector<std::pair<TShared<Entity>, IScript::Request::Ref> > listeners;

		while (!nextFrameListeners.Empty()) {
			std::pair<TShared<Entity>, IScript::Request::Ref>& entry = nextFrameListeners.Top();
			listeners.emplace_back(std::move(entry));
			entry.first = nullptr;
			nextFrameListeners.Pop();
		}

		engine.TickFrame();

		ThreadPool& threadPool = engine.bridgeSunset.GetThreadPool();
		if (threadPool.GetTaskCount() > 0) {
			threadPool.BalanceUp();
		} else {
			threadPool.BalanceDown();
		}

		for (size_t k = 0; k < listeners.size(); k++) {
			std::pair<TShared<Entity>, IScript::Request::Ref>& entry = listeners[k];
			engine.GetKernel().QueueRoutine(entry.first(), CreateTaskScriptOnce(entry.second));
		}
	}
}

uint64_t MythForest::RequestGetFrameTickDelta(IScript::Request& request) {
	return engine.GetFrameTickDelta();
}

Float3Pair MythForest::RequestGetEntityBoundingBox(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	return entity->GetKey();
}

void MythForest::RequestWaitForNextFrame(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);

	request.DoLock();
	nextFrameListeners.Push(std::make_pair(entity.Get(), callback));
	request.UnLock();
}

TShared<Entity> MythForest::RequestNewEntity(IScript::Request& request, int32_t warp) {
	return CreateEntity(warp);
}

void MythForest::RequestEnumerateComponentModules(IScript::Request& request) {
	request << begintable;
	const std::unordered_map<String, Module*>& subModules = engine.GetModuleMap();
	for (std::unordered_map<String, Module*>::const_iterator it = subModules.begin(); it != subModules.end(); ++it) {
		request << key((*it).first) << *((*it).second);
	}

	request << endtable;
}

void MythForest::RequestAddEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(entity);
	
	if (!(component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_OVERRIDE_WARP)) {
		CHECK_THREAD_IN_MODULE(component);
	}

	entity->AddComponent(engine, component.Get());
}

void MythForest::RequestUpdateEntity(IScript::Request& request, IScript::Delegate<Entity> entity, bool recursive) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	entity->UpdateEntityFlags();
	entity->UpdateBoundingBox(engine, recursive);
}

void MythForest::RequestRemoveEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(entity);

	entity->RemoveComponent(engine, component.Get());
}

TShared<Component> MythForest::RequestGetUniqueEntityComponent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& componentName) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	Module* module = engine.GetComponentModuleFromName(componentName);
	if (module != nullptr) {
		Component* component = module->GetEntityUniqueComponent(entity.Get()); // Much more faster 
		if (component != nullptr) {
			return component;
		}
	} else {
		size_t size = entity->GetComponentCount();
		for (size_t i = 0; i < size; i++) {
			Component* component = entity->GetComponent(i);
			if (component != nullptr && (component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_ALIASED_TYPE)) {
				if (component->GetAliasedTypeName() == componentName) {
					return component;
				}
			}
		}
	}

	return nullptr;
}

void MythForest::RequestGetEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	size_t size = entity->GetComponentCount();
	request << beginarray;

	for (size_t i = 0; i < size; i++) {
		Component* component = entity->GetComponent(i);
		if (component != nullptr) {
			request << component;
		}
	}

	request << endarray;
}

String MythForest::RequestGetComponentType(IScript::Request& request, IScript::Delegate<Component> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);
	CHECK_THREAD_IN_MODULE(component);

	if (component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_ALIASED_TYPE) {
		return component->GetAliasedTypeName();
	} else {
		return component->GetUnique()->GetBriefName();
	}
}

void MythForest::RequestClearEntityComponents(IScript::Request& request, IScript::Delegate<Entity> entity) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	entity->ClearComponents(engine);
}

class ScriptRaycastTask : public Component::RaycastTaskWarp {
public:
	ScriptRaycastTask(Engine& engine, uint32_t maxCount, IScript::Request::Ref ref) : RaycastTaskWarp(engine, maxCount), callback(ref) {}

	void Finish(rvalue<std::vector<Component::RaycastResult> > r) override {
		std::vector<Component::RaycastResult>& results = r;
		IScript::Request& request = *engine.bridgeSunset.requestPool.AcquireSafe();

		request.DoLock();
		request.Push();
		request << beginarray;
		for (uint32_t i = 0; i < results.size(); i++) {
			const Component::RaycastResult& result = results[i];
			request << begintable
				<< key("Intersection") << result.position
				<< key("TexCoord") << result.coord
				<< key("Distance") << sqrtf(result.squareDistance)
				<< key("Object") << result.unit
				<< key("Parent") << result.parent
				<< endtable;
		}
		request << endarray;
		request.Call(callback);
		request.Pop();
		request.Dereference(callback);
		request.UnLock();

		engine.bridgeSunset.requestPool.ReleaseSafe(&request);
	}

protected:
	IScript::Request::Ref callback;
};

void MythForest::RequestRaycast(IScript::Request& request, IScript::Delegate<Entity> entity, IScript::Request::Ref callback, const Float3& from, const Float3& dir, uint32_t count) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	TShared<ScriptRaycastTask> task = TShared<ScriptRaycastTask>::From(new ScriptRaycastTask(engine, count, callback));
	task->AddPendingTask();
	Float3Pair ray(from, dir);
	MatrixFloat4x4 transform = MatrixFloat4x4::Identity();
	Component::RaycastForEntity(*task(), Math::QuickRay(ray), ray, transform, entity.Get(), 1.0f);
	task->RemovePendingTask();
}

void MythForest::RequestCaptureFrame(IScript::Request& request, const String& path, const String& options) {
	InvokeCaptureFrame(path, options);
}

void MythForest::RequestPostEvent(IScript::Request& request, IScript::Delegate<Entity> entity, const String& type, IScript::Delegate<SharedTiny> sender, IScript::Request::Ref param) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(entity);
	CHECK_THREAD_IN_MODULE(entity);

	TShared<TSharedTinyWrapper<IScript::Request::Ref> > wrapper;
	if (param) {
		wrapper.Reset(new TSharedTinyWrapper<IScript::Request::Ref>(param));
	}

	// now we only support post custom events and update event.
	Event event(engine, type == "Update" ? Event::EVENT_UPDATE : Event::EVENT_CUSTOM, sender.Get(), wrapper());
	entity->PostEvent(event, ~0);

	if (param) {
		request.DoLock();
		request.Dereference(param);
		request.UnLock();
	}
}

void MythForest::OnSize(const Int2& size) {
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnSize(size);
}

void MythForest::OnMouse(const IFrame::EventMouse& mouse) {
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnMouse(mouse);
}

void MythForest::OnKeyboard(const IFrame::EventKeyboard& keyboard) {
	EventComponentModule& eventComponentModule = *engine.GetComponentModuleFromName("EventComponent")->QueryInterface(UniqueType<EventComponentModule>());
	eventComponentModule.OnKeyboard(keyboard);
}

void MythForest::StartCaptureFrame(const String& path, const String& options) {
#if USE_FRAME_CAPTURE
	debugger.SetDumpHandler(path, TWrapper<bool>());
	debugger.StartDump(options);
#endif
}

void MythForest::EndCaptureFrame() {
#if USE_FRAME_CAPTURE
	debugger.EndDump();
#endif
}

void MythForest::InvokeCaptureFrame(const String& path, const String& options) {
#if USE_FRAME_CAPTURE
	debugger.SetDumpHandler(path, TWrapper<bool>());
	debugger.InvokeDump(options);
#endif
}
