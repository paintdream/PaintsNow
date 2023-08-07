#include "VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <cmath>

using namespace PaintsNow;

VisibilityComponent::VisibilityComponent(const TShared<StreamComponent>& stream) : gridSize(0.5f, 0.5f, 0.5f), viewDistance(512.0f), taskCount(32), resolution(128, 128), activeCellCacheIndex(0), pixelKillThreshold(1), hostEntity(nullptr), renderQueue(nullptr), depthStencilResource(nullptr), stateResource(nullptr), collectLock(nullptr), streamComponent(stream) {
	pendingSetupCount.store(0, std::memory_order_relaxed);
	maxVisIdentity.store(0, std::memory_order_relaxed);
	cellAllocator.Reset(new TObjectAllocator<Cell>());
}

Tiny::FLAG VisibilityComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

TObject<IReflect>& VisibilityComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(gridSize);
		ReflectProperty(viewDistance);
		ReflectProperty(resolution);
		ReflectProperty(taskCount);
		ReflectProperty(pixelKillThreshold);
	}

	return *this;
}

void VisibilityComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	assert(hostEntity == nullptr);
	assert(renderQueue == nullptr);
	hostEntity = entity;

	// Allocate IDs
	maxVisIdentity.store(1, std::memory_order_relaxed);
	pendingSetupCount.store(0, std::memory_order_relaxed);

	size_t size = hostEntity->GetComponentCount();
	for (size_t n = 0; n < size; n++) {
		Component* component = hostEntity->GetComponent(n);
		if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
			SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
			SetupIdentities(engine, spaceComponent->GetRootEntity());
		}
	}

	IThread& thread = engine.interfaces.thread;
	collectLock = thread.NewLock();

	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	renderQueue = render.CreateQueue(device);

	String path = ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName();
	pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

	stateResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
	IRender::Resource::RenderStateDescription& rs = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(renderQueue, stateResource, 0));
	rs.stencilReplacePass = 1;
	rs.cull = 0;
	rs.fill = 1;
	rs.blend = 0;
	rs.colorWrite = 1;
	rs.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	rs.depthWrite = 1;
	rs.stencilTest = 0;
	rs.stencilWrite = 0;
	rs.stencilValue = 0;
	rs.stencilMask = 0;
	render.UnmapResource(renderQueue, stateResource, IRender::MAP_DATA_EXCHANGE);

	UShort3 dim(resolution.x(), resolution.y(), 0u);
	depthStencilResource = render.CreateResource(device, IRender::Resource::RESOURCE_TEXTURE);
	IRender::Resource::TextureDescription& depthStencilDescription = *static_cast<IRender::Resource::TextureDescription*>(render.MapResource(renderQueue, depthStencilResource, 0));
	depthStencilDescription.dimension = dim;

	bool supportIntegratedD24S8Format = render.GetProfile(render.GetQueueDevice(renderQueue), "DepthFormat_D24S8") != 0;
	depthStencilDescription.state.format = (uint32_t)(supportIntegratedD24S8Format ? IRender::Resource::TextureDescription::HALF : IRender::Resource::TextureDescription::FLOAT);
	depthStencilDescription.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	depthStencilDescription.state.attachment = 1;
	render.UnmapResource(renderQueue, depthStencilResource, IRender::MAP_DATA_EXCHANGE);

	tasks.resize(taskCount);

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		task.renderQueue = render.CreateQueue(device);

		TShared<TextureResource>& texture = task.texture;
		texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("VisBake", &task), false, ResourceBase::RESOURCE_VIRTUAL);
		texture->description.dimension = dim;
		texture->description.state.attachment = true;
		texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		texture->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		texture->description.state.attachment = 1;
		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		texture->GetResourceManager().InvokeUpload(texture(), renderQueue);

		task.renderTarget = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
		IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(renderQueue, task.renderTarget, 0));
		desc.colorStorages.resize(1);
		IRender::Resource::RenderTargetDescription::Storage& s = desc.colorStorages[0];
		s.resource = texture->GetRenderResource();
		s.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		s.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
		desc.depthStorage.resource = depthStencilResource;
		desc.depthStorage.loadOp = desc.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
		desc.dimension = dim;

		render.SetResourceNote(task.renderTarget, "VisibilityBakePass");
		render.UnmapResource(renderQueue, task.renderTarget, IRender::MAP_DATA_EXCHANGE);
		task.warpData.resize(engine.GetKernel().GetWarpCount());
	}

	streamComponent->SetLoadHandler(Wrap(this, &VisibilityComponent::StreamLoadHandler));
	streamComponent->SetUnloadHandler(Wrap(this, &VisibilityComponent::StreamUnloadHandler));

	BaseComponent::Initialize(engine, entity);
}

void VisibilityComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	// TODO: wait for all download tasks to finish.
	assert(hostEntity != nullptr);
	streamComponent->SetLoadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>&>());
	streamComponent->SetUnloadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>&>());

	hostEntity = nullptr;
	IRender& render = engine.interfaces.render;
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];

		render.DeleteResource(task.renderQueue, task.renderTarget);
		render.DeleteQueue(task.renderQueue);
	}

	tasks.clear();

	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, depthStencilResource);
	render.DeleteQueue(renderQueue);

	stateResource = depthStencilResource = nullptr;
	renderQueue = nullptr;

	engine.interfaces.thread.DeleteLock(collectLock);
	collectLock = nullptr;
	BaseComponent::Uninitialize(engine, entity);
}

Entity* VisibilityComponent::GetHostEntity() const {
	return hostEntity;
}

void VisibilityComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (event.eventID == Event::EVENT_FRAME) {
		// DoBake
		TickRender(event.engine);

		// Prepare
		Engine& engine = event.engine;
		engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &VisibilityComponent::RoutineTickTasks), std::ref(engine)));
	}
}

void VisibilityComponent::Setup(Engine& engine, float distance, const Float3& size, uint32_t tc, uint32_t pc, const UShort2& res) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_MODIFIED));

	taskCount = tc;
	gridSize = size;
	viewDistance = distance;
	resolution = res;
	pixelKillThreshold = verify_cast<uint16_t>(pc);

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void VisibilityComponent::RoutineSetupIdentities(Engine& engine, Entity* entity) {
	SetupIdentities(engine, entity);
	pendingSetupCount.fetch_sub(1, std::memory_order_release);
}

void VisibilityComponent::SetupIdentities(Engine& engine, Entity* entity) {
	for (Entity* p = entity; p != nullptr; p = p->Right()) {
		TransformComponent* transformComponent = p->GetUniqueComponent(UniqueType<TransformComponent>());
		if (transformComponent != nullptr) {
			assert(transformComponent->GetObjectID() == 0);
			transformComponent->SetObjectID(maxVisIdentity.fetch_add(1, std::memory_order_relaxed));
		}

		size_t size = p->GetComponentCount();
		for (size_t i = 0; i < size; i++) {
			Component* component = p->GetComponent(i);
			if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
				if (component->GetWarpIndex() == GetWarpIndex()) {
					SetupIdentities(engine, static_cast<SpaceComponent*>(component)->GetRootEntity());
				} else {
					pendingSetupCount.fetch_add(1, std::memory_order_relaxed);
					// engine.GetKernel().QueueBarrier(GetWarpIndex());
					engine.GetKernel().QueueRoutine(component, CreateTaskContextFree(Wrap(this, &VisibilityComponent::RoutineSetupIdentities), std::ref(engine), static_cast<SpaceComponent*>(component)->GetRootEntity()));
				}
			}
		}

		if (p->Left() != nullptr) {
			SetupIdentities(engine, p->Left());
		}
	}
}

static inline void MergeSample(Bytes& dst, const Bytes& src) {
	size_t s = src.GetSize();
	const uint8_t* f = src.GetData();
	uint8_t* t = dst.GetData();

	for (uint32_t i = 0; i < s; i++) {
		t[i] |= f[i];
	}
}

VisibilityComponentConfig::Cell::Cell() {
	dispatched.store(0, std::memory_order_relaxed);
	finished.store(0, std::memory_order_relaxed);
}

const Bytes& VisibilityComponent::QuerySample(Engine& engine, const Float3& position) {
	OPTICK_EVENT();

	if (pendingSetupCount.load(std::memory_order_acquire) != 0)
		return Bytes::Null();

	assert(engine.GetKernel().GetCurrentWarpIndex() == GetWarpIndex());
	Int3 intPosition(int32_t(floorf(position.x() / gridSize.x())), int32_t(floorf(position.y() / gridSize.y())), int32_t(floorf(position.z() / gridSize.z())));
	const UShort3& dimension = streamComponent->GetDimension();
	assert(dimension.x() >= 4 && dimension.y() >= 4 && dimension.z() >= 4);
	assert(streamComponent->GetCacheCount() >= 64);

	// load cache
	const uint32_t cacheCount = sizeof(cellCache) / sizeof(cellCache[0]);
	for (uint16_t i = activeCellCacheIndex; i < cacheCount + activeCellCacheIndex; i++) {
		Cache& current = cellCache[i % cacheCount];
		if (!current.payload.Empty() && current.intPosition == intPosition) {
			// hit!
			return current.payload;
		}
	}

	std::vector<TShared<Cell> > toMerge;
	toMerge.reserve(8);
	uint32_t mergedSize = 0;
	bool incomplete = false;

	for (int32_t z = intPosition.z(); z <= intPosition.z() + 1; z++) {
		for (int32_t y = intPosition.y(); y <= intPosition.y() + 1; y++) {
			for (int32_t x = intPosition.x(); x <= intPosition.x() + 1; x++) {
				Int3 coord(x, y, z);
				TShared<Cell> cell = streamComponent->Load(engine, streamComponent->ComputeWrapCoordinate(coord), nullptr)->QueryInterface(UniqueType<Cell>());

				bool curIncomplete = cell->finished.load(std::memory_order_acquire) != Cell::ALL_FACE_MASK;
				if (curIncomplete) {
					cell->intPosition = coord;
					BinaryInsert(bakePoints, cell);
				}

				incomplete = incomplete || curIncomplete;
				toMerge.emplace_back(cell);
				mergedSize = Math::Max(mergedSize, (uint32_t)verify_cast<uint32_t>(cell->payload.GetSize()));
			}
		}
	}

	if (incomplete) return Bytes::Null();

	// Do Merge
	Bytes mergedPayload;
	mergedPayload.Resize(mergedSize, 0);
	for (size_t j = 0; j < toMerge.size(); j++) {
		MergeSample(mergedPayload, toMerge[j]->payload);
	}

	Cache& targetCache = cellCache[activeCellCacheIndex];
	targetCache.intPosition = intPosition;
	targetCache.payload = std::move(mergedPayload);
	activeCellCacheIndex = (activeCellCacheIndex + 1) % (sizeof(cellCache) / sizeof(cellCache[0]));
	return targetCache.payload;
}

TShared<SharedTiny> VisibilityComponent::StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	OPTICK_EVENT();
	return tiny ? tiny->QueryInterface(UniqueType<Cell>()) : TShared<SharedTiny>::From(cellAllocator->New());
}

TShared<SharedTiny> VisibilityComponent::StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	TShared<Cell> cell = tiny->QueryInterface(UniqueType<Cell>());

	// is ready?
	if (cell->finished.load(std::memory_order_acquire) == Cell::ALL_FACE_MASK) {
		cell->payload.Clear();
		cell->dispatched.store(0, std::memory_order_release);
		cell->finished.store(0, std::memory_order_release);
		cell->Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);

		return cell();
	} else {
		return nullptr; // let it choose a new one
	}
}

bool VisibilityComponent::IsVisible(const Bytes& s, TransformComponent* transformComponent) {
	if (s.Empty()) return true;
	uint32_t id = transformComponent->GetObjectID();

	uint32_t offset = id >> 3;
	size_t size = s.GetSize();
	if (offset >= size) return false;
	const uint8_t* ptr = s.GetData();
	uint8_t testBit = 1 << (id & 7);

	assert(offset < size);
	return !!(ptr[offset] & testBit);
}

// Baker

void VisibilityComponent::TickRender(Engine& engine) {
	OPTICK_EVENT();
	// check pipeline state
	if (renderQueue == nullptr) return; // not inited.
	IRender& render = engine.interfaces.render;
	render.SubmitQueues(&renderQueue, 1, IRender::SUBMIT_EXECUTE_ALL);

	// read previous results back and collect ready ones
	std::vector<IRender::Queue*> bakeQueues;
	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		TextureResource* texture = task.texture();
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		IRender::Resource::TextureDescription* p = nullptr;
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			if (task.Continue()) {
				// printf("REQUEST %p\n", texture->GetRenderResource());
				p = static_cast<IRender::Resource::TextureDescription*>(render.MapResource(task.renderQueue, texture->GetRenderResource(), IRender::MAP_DATA_EXCHANGE));
				if (p == nullptr) { // currently not available, just wait
					render.FlushQueue(task.renderQueue);
					bakeQueues.emplace_back(task.renderQueue);
					finalStatus.store(TaskData::STATUS_BAKING, std::memory_order_release);
				}
			} else {
				// failed!!
				task.cell->dispatched.fetch_and(~(1 << task.faceIndex), std::memory_order_relaxed);
				finalStatus.store(TaskData::STATUS_IDLE, std::memory_order_release);
			}
		} else if (task.status == TaskData::STATUS_BAKING) {
			p = static_cast<IRender::Resource::TextureDescription*>(render.MapResource(task.renderQueue, texture->GetRenderResource(), IRender::MAP_DATA_EXCHANGE));
		}

		if (p != nullptr) {
			bakeQueues.emplace_back(task.renderQueue);
			task.data = std::move(p->data);
			render.UnmapResource(task.renderQueue, texture->GetRenderResource(), 0);
			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.SubmitQueues(&bakeQueues[0], verify_cast<uint32_t>(bakeQueues.size()), IRender::SUBMIT_EXECUTE_CONSUME);
	}
}

TObject<IReflect>& VisibilityComponentConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}

VisibilityComponentConfig::InstanceGroup::InstanceGroup() : instanceCount(0) {}

void VisibilityComponentConfig::InstanceGroup::Cleanup() {
	for (size_t k = 0; k < instancedData.size(); k++) {
		instancedData[k].Clear();
	}

	instanceCount = 0;
}

VisibilityComponentConfig::MergedInstanceGroup::MergedInstanceGroup() : groupPrototype(nullptr), mergedInstanceCount(0) {}

void VisibilityComponent::CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData, uint32_t identity) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = task.renderQueue;
	const UChar4& encode = *reinterpret_cast<const UChar4*>(&identity);
	instanceData.instancedColor = Float4((float)encode[0], (float)encode[1], (float)encode[2], (float)encode[3]) / 255.0f;
	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	TaskData::WarpData& warpData = task.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];

	IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
	IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
	std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
	IThread& thread = engine.interfaces.thread;
	thread.DoLock(collectLock);
	uint32_t count = renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_DEFAULT);

	if (count == ~(uint32_t)0) { // failed?
		task.pendingResourceCount++;
	}

	thread.UnLock(collectLock);

	if (task.pendingResourceCount != 0)
		return;

	TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;
	for (size_t k = 0; k < drawCalls.size(); k++) {
		// PassBase& Pass = provider->GetPass(k);
		IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
		const IRender::Resource::QuickDrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;

		// Generate key
		InstanceKey key = (InstanceKey)drawCall.hashValue;
		InstanceGroup& group = instanceGroups[key];
		if (group.instanceCount == 0) {
			group.drawCallDescription = drawCallTemplate;
			PassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();
			instanceData.Export(group.instanceUpdater, updater);
		}

		// drop textures and buffer resources
		std::vector<IRender::Resource*> textureResources;
		std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
		group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
		group.instanceCount++;
	}
}

void VisibilityComponent::CompleteCollect(Engine& engine, TaskData& task) {}

void VisibilityComponent::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	if (transformComponent != nullptr) {
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if (!captureData(localBoundingBox))
			return;
	}

	WorldInstanceData instanceData;
	instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;

	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	TaskData::WarpData& warpData = task.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];
	ExplorerComponent::ComponentPointerAllocator allocator(&warpData.bytesCache);
	std::vector<Component*, ExplorerComponent::ComponentPointerAllocator> exploredComponents(allocator);
	ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());

	singleton Unique expectedIdentifier = UniqueType<RenderableComponent>::Get();
	if (explorerComponent != nullptr && explorerComponent->GetExploreIdentifier() == expectedIdentifier) {
		// Use nearest refValue for selecting most detailed components
		explorerComponent->SelectComponents(engine, entity, 0.0f, exploredComponents);
	} else {
		exploredComponents.reserve(entity->GetComponentCount());
		entity->CollectComponents(exploredComponents);
	}

	for (size_t k = 0; k < exploredComponents.size(); k++) {
		Component* component = exploredComponents[k];
		assert(component != nullptr);

		if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERABLE) {
			if (transformComponent != nullptr) {
				RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
				if (!(renderableComponent->Flag().load(std::memory_order_acquire) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
					if (renderableComponent->GetVisible()) {
						CollectRenderableComponent(engine, task, renderableComponent, instanceData, transformComponent->GetObjectID());
					}
				}
			}
		} else if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
			std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
			counter.fetch_add(1, std::memory_order_acquire);
			SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
			if (transformComponent != nullptr) {
				CaptureData newCaptureData;
				task.camera.UpdateCaptureData(newCaptureData, captureData.viewTransform * Math::QuickInverse(transformComponent->GetTransform()));
				CollectComponentsFromSpace(engine, task, instanceData, newCaptureData, spaceComponent);
			} else {
				CollectComponentsFromSpace(engine, task, instanceData, captureData, spaceComponent);
			}
		}
	}
}

void VisibilityComponent::PostProcess(TaskData& task) {
	OPTICK_EVENT();
	assert(task.status == TaskData::STATUS_POSTPROCESS);
	Cell& cell = *task.cell;
	Bytes& encodedData = cell.payload;
	const Bytes& data = task.data;
	assert(data.GetSize() % sizeof(uint32_t) == 0);
	uint32_t count = verify_cast<uint32_t>(data.GetSize()) / sizeof(uint32_t);
	const uint32_t* p = reinterpret_cast<const uint32_t*>(data.GetData());

	uint32_t idCount = maxVisIdentity.load(std::memory_order_acquire);
	std::vector<uint16_t> counters(idCount, 0);
	uint32_t maxVisibleID = 0;
	for (uint32_t m = 0; m < count; m++) {
		uint32_t objectID = p[m];
		if (objectID != 0 && objectID < idCount) {
			uint16_t& record = counters[objectID];
			if (record < 0xffff) {
				record++;
				maxVisibleID = Math::Max(maxVisibleID, objectID);
			}
		}
	}

	uint32_t size = maxVisibleID / 8 + 1;
	if (encodedData.GetSize() < size) {
		encodedData.Resize(size, 0);
	}

	uint8_t* target = encodedData.GetData();
	for (uint32_t k = 0; k <= maxVisibleID; k++) {
		uint16_t record = counters[k];
		if (record > pixelKillThreshold) {
			uint32_t location = k >> 3;
			target[location] |= 1 << (k & 7);
		}
	}

	cell.finished.fetch_or(1 << task.faceIndex, std::memory_order_release);

	// cell finished?
	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
	finalStatus.store(TaskData::STATUS_IDLE, std::memory_order_release);
}

void VisibilityComponent::ResolveTasks(Engine& engine) {
	OPTICK_EVENT();
	// resolve finished tasks
	for (size_t n = 0; n < tasks.size(); n++) {
		TaskData& taskData = tasks[n];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(taskData.status);
		if (taskData.status == TaskData::STATUS_BAKED) {
			finalStatus.store(TaskData::STATUS_POSTPROCESS);
			engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &VisibilityComponent::PostProcess), std::ref(taskData)));
		} else if (taskData.status == TaskData::STATUS_ASSEMBLING) {
			if (taskData.pendingCount == 0) {
				// Commit draw calls.
				if (taskData.Continue()) {
					IRender::Queue* queue = taskData.renderQueue;
					IRender& render = engine.interfaces.render;

					IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);

					// Merge all instance group
#if defined(_MSC_VER) && _MSC_VER <= 1200
					TaskData::MergedInstanceGroupMap mergedInstancedGroupMap;
#else
					TaskData::MergedInstanceGroupMap mergedInstancedGroupMap(&taskData.globalBytesCache);
#endif

					for (size_t m = 0; m < taskData.warpData.size(); m++) {
						TaskData::WarpData& warpData = taskData.warpData[m];
						TaskData::WarpData::InstanceGroupMap& instanceGroup = warpData.instanceGroups;
						for (TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroup.begin(); it != instanceGroup.end(); ++it) {
							InstanceKey key = (*it).first;
							InstanceGroup& group = (*it).second;
							if (group.instanceCount == 0)
								continue;

							assert(group.drawCallDescription.GetShader() != nullptr);
							MergedInstanceGroup& merged = mergedInstancedGroupMap[key];
							if (merged.groupPrototype == nullptr) {
								merged.groupPrototype = &group;
								merged.mergedInstanceData.resize(group.instancedData.size());
							}

							merged.mergedInstanceCount += group.instanceCount;

							for (size_t i = 0; i < merged.mergedInstanceData.size(); i++) {
								Bytes& instanceData = group.instancedData[i];
								if (!instanceData.Empty()) {
									Bytes data = taskData.globalBytesCache.New(verify_cast<uint32_t>(group.instancedData[i].GetViewSize()));
									data.Import(0, group.instancedData[i]);
									taskData.globalBytesCache.Link(merged.mergedInstanceData[i], data);
								}
							}
						}
					}

					Bytes instanceData;
					uint32_t instanceOffset = 0;
					std::vector<IRender::Resource*> drawCallResources;
					// Generate all render resources.
					for (TaskData::MergedInstanceGroupMap::iterator it = mergedInstancedGroupMap.begin(); it != mergedInstancedGroupMap.end(); ++it) {
						MergedInstanceGroup& mergedGroup = (*it).second;
						InstanceGroup& group = *mergedGroup.groupPrototype;
						assert(group.drawCallDescription.GetShader() != nullptr && group.instanceCount != 0);
						group.drawCallDescription.instanceCount = mergedGroup.mergedInstanceCount;

						// Concat instance data
						std::vector<Bytes>& mergedInstanceData = (*it).second.mergedInstanceData;
						uint32_t mergedInstanceCount = (*it).second.mergedInstanceCount;

						for (size_t k = 0; k < mergedInstanceData.size(); k++) {
							Bytes& data = mergedInstanceData[k];
							if (!data.Empty()) {
								// assign instanced buffer	
								size_t viewSize = data.GetViewSize();
								IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.GetBuffers()[k];

								bufferRange.buffer = buffer;
								bufferRange.offset = instanceOffset; // TODO: alignment
								bufferRange.length = 0;
								bufferRange.component = verify_cast<uint16_t>(viewSize / (mergedInstanceCount * sizeof(float)));
								bufferRange.type = 0;
								taskData.globalBytesCache.Link(instanceData, data);
								instanceOffset += verify_cast<uint32_t>(viewSize);
								assert(instanceOffset == instanceData.GetViewSize());
							}
						}

						assert(PassBase::ValidateDrawCall(group.drawCallDescription));
						IRender::Resource* drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_QUICK_DRAWCALL, queue);
						IRender::Resource::QuickDrawCallDescription& dc = *static_cast<IRender::Resource::QuickDrawCallDescription*>(render.MapResource(queue, drawCall, 0));
						dc = group.drawCallDescription; // make copy
						render.UnmapResource(queue, drawCall, IRender::MAP_DATA_EXCHANGE);
						drawCallResources.emplace_back(drawCall);
					}

					IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
					assert(instanceOffset == instanceData.GetViewSize());
					desc.data.Resize(instanceOffset);
					desc.data.Import(0, instanceData);
					desc.state.format = IRender::Resource::BufferDescription::FLOAT;
					desc.state.usage = IRender::Resource::BufferDescription::INSTANCED;
					desc.state.dynamic = 1;
					desc.state.component = 0;
					render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);

					for (size_t j = 0; j < drawCallResources.size(); j++) {
						IRender::Resource* drawCall = drawCallResources[j];
						render.ExecuteResource(queue, drawCall);
						// cleanup at current frame
						render.DeleteResource(queue, drawCall);
					}

					render.DeleteResource(queue, buffer);
				}

				// Cleanup
				taskData.globalBytesCache.Reset();
				for (size_t k = 0; k < taskData.warpData.size(); k++) {
					TaskData::WarpData& warpData = taskData.warpData[k];
					for (TaskData::WarpData::InstanceGroupMap::iterator it = warpData.instanceGroups.begin(); it != warpData.instanceGroups.end();) {
						InstanceGroup& group = (*it).second;

						if (group.instanceCount == 0) {
							// to be deleted.
							warpData.instanceGroups.erase(it++);
						} else {
							group.Cleanup();
							++it;
						}
					}

					warpData.bytesCache.Reset();
				}

				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void VisibilityComponent::CoTaskAssembleTask(Engine& engine, TaskData& task, const TShared<VisibilityComponent>& selfHolder) {
	OPTICK_EVENT();
	if (hostEntity == nullptr) return;

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	const float t = 0.1f;
	const float f = 5000.0f;

	const float projectMatValues[] = {
		1.0f, 0, 0, 0,
		0, 1.0f, 0, 0,
		0.0f, 0.0f, (f + t) / (f - t), -1,
		0, 0, 2 * f * t / (f - t), 0
	};
	const double PI = 3.14159265358979323846;

	const MatrixFloat4x4 projectionMatrix(projectMatValues);
	PerspectiveCamera camera;
	camera.aspect = 1.0f;
	camera.farPlane = 5000.0f;
	camera.nearPlane = 0.1f;
	camera.fov = (float)PI / 2;
	task.camera = camera;

	const float r2 = (float)sqrt(2.0f) / 2.0f;
	static const Float3 directions[Cell::FACE_COUNT] = {
		Float3(r2, r2, 0),
		Float3(-r2, r2, 0),
		Float3(-r2, -r2, 0),
		Float3(r2, -r2, 0),
		Float3(0, 0, 1),
		Float3(0, 0, -1)
	};

	static const Float3 ups[Cell::FACE_COUNT] = {
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(1, 0, 0),
		Float3(1, 0, 0),
	};

	const Int3& intPosition = task.cell->intPosition;
	Float3 viewPosition((intPosition.x() + 0.5f) * gridSize.x(), (intPosition.y() + 0.5f) * gridSize.y(), (intPosition.z() + 0.5f) * gridSize.z());

	CaptureData captureData;
	MatrixFloat4x4 viewMatrix = Math::MatrixLookAt(viewPosition, directions[task.faceIndex], ups[task.faceIndex]);
	MatrixFloat4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

	camera.UpdateCaptureData(captureData, Math::QuickInverse(viewMatrix));

	WorldInstanceData instanceData;
	instanceData.worldMatrix = viewProjectionMatrix;
	CollectComponentsFromEntity(engine, task, instanceData, captureData, hostEntity);

	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);
}

void VisibilityComponent::DispatchTasks(Engine& engine) {
	OPTICK_EVENT();
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	for (size_t i = 0, n = 0; i < bakePoints.size() && n < tasks.size(); i++) {
		const TShared<Cell>& cell = bakePoints[i];
		Entity* entity = hostEntity;

		for (uint32_t k = 0; k < Cell::FACE_COUNT; k++) {
			if (!(cell->dispatched.load(std::memory_order_relaxed) & (1 << k))) {
				while (n < tasks.size()) {
					TaskData& task = tasks[n++];

					if (task.status == TaskData::STATUS_START) {
						task.cell = cell;
						task.faceIndex = k;
						task.pendingResourceCount = 0;
						cell->dispatched.fetch_or(1 << k, std::memory_order_relaxed);

						std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
						finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_release);
						threadPool.Dispatch(CreateCoTaskContextFree(kernel, Wrap(this, &VisibilityComponent::CoTaskAssembleTask), std::ref(engine), std::ref(task), this));
						break;
					}
				}

				if (n == tasks.size())
					break; // not enough slots
			}
		}
	}

	bakePoints.clear();
}

void VisibilityComponent::RoutineTickTasks(Engine& engine) {
	OPTICK_EVENT();

	// Must complete all pending resources
	if (engine.snowyStream.GetRenderResourceManager()->GetCompleted()) {
		ResolveTasks(engine);
		DispatchTasks(engine);
	}
}
