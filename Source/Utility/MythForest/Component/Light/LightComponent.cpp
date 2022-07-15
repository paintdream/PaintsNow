#include "LightComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../../../MythForest/MythForest.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

LightComponent::LightComponent() : attenuation(0), range(0, 0, 0) /*, spotAngle(1), temperature(6500) */ {}

Tiny::FLAG LightComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERCONTROL | RenderableComponent::GetEntityFlagMask();
}

uint32_t LightComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	return 0;
}

TObject<IReflect>& LightComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(color);
		ReflectProperty(attenuation);
		/*
		ReflectProperty(spotAngle);
		ReflectProperty(temperature);*/
	}

	return *this;
}

void LightComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			shadowLayer->Uninitialize(engine);
		}
	}

	BaseClass::Uninitialize(engine, entity);
}

void LightComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {
	Math::Union(box, Float3(-range));
	Math::Union(box, range);
}

const std::vector<TShared<LightComponent::ShadowLayer> >& LightComponent::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	OPTICK_EVENT();
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			shadowLayer->UpdateShadow(engine, cameraTransform, lightTransform, rootEntity);
		} 
	}

	return shadowLayers;
}

void LightComponent::BindShadowStream(Engine& engine, uint32_t layer, const TShared<StreamComponent>& streamComponent, const UShort2& res, float size, float scale) {
	OPTICK_EVENT();
	if (shadowLayers.size() <= layer) {
		shadowLayers.resize(layer + 1);
	}

	TShared<ShadowLayer>& shadowLayer = shadowLayers[layer];
	if (!shadowLayer) {
		shadowLayer = TShared<ShadowLayer>::From(new ShadowLayer(engine));
	}
	
	shadowLayer->Initialize(engine, streamComponent, res, size, scale);
}

LightComponent::ShadowLayer::ShadowLayer(Engine& engine) : gridSize(1), scale(1) {}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	OPTICK_EVENT();
	assert(context);

	TShared<ShadowGrid> shadowGrid;

	// reuse?
	if (tiny && tiny != currentGrid) {
		shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
		assert(shadowGrid);
	} else {
		shadowGrid = TShared<ShadowGrid>::From(new ShadowGrid());
		shadowGrid->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}

	return shadowGrid();
}

void LightComponent::ShadowLayer::StreamRefreshHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	if (!(tiny->Flag().load(std::memory_order_acquire) & TINY_MODIFIED))
		return;
	
	if (!engine.snowyStream.GetRenderResourceManager()->GetCompleted())
		return;

	TShared<TaskData>& taskData = currentTask;
	if (taskData->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed) & TINY_MODIFIED)
		return;

	taskData->Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
	uint32_t ret = tiny->Flag().fetch_or(TINY_UPDATING, std::memory_order_release);
	assert(!(ret & TINY_UPDATING));

	TShared<ShadowGrid> shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
	UShort3 dim(resolution.x(), resolution.y(), 1);

	if (!shadowGrid->texture) {
		IRender::Resource::TextureDescription depthStencilDescription;
		depthStencilDescription.dimension = dim;
		depthStencilDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
		depthStencilDescription.state.layout = IRender::Resource::TextureDescription::DEPTH;

		TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("LightShadowDepth", tiny()), false, ResourceBase::RESOURCE_VIRTUAL);
		texture->description.dimension = dim;
		texture->description.state.attachment = true;
		texture->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		texture->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		texture->description.state.addressU = texture->description.state.addressV = texture->description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
		texture->description.state.sample = IRender::Resource::TextureDescription::POINT;
		texture->description.state.pcf = true;
		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		texture->GetResourceManager().InvokeUpload(texture(), taskData->renderQueue);
		shadowGrid->texture = texture;
	}

	// get entity
	ShadowContext* shadowContext = context->QueryInterface(UniqueType<ShadowContext>());
	assert(shadowContext != nullptr);

	// calculate position
	CaptureData captureData;
	const MatrixFloat4x4& viewMatrix = shadowContext->lightTransformMatrix;
	OrthoCamera::UpdateCaptureData(captureData, viewMatrix);
	WorldInstanceData instanceData;
	instanceData.worldMatrix = shadowGrid->shadowMatrix = Math::QuickInverse(Math::MatrixScale(Float4(1, -1, -1, 1)) * viewMatrix);
	IRender& render = engine.interfaces.render;

	taskData->rootEntity = shadowContext->rootEntity; // in case of gc
	taskData->shadowGrid = shadowGrid();

	// Prepare render target
	IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(taskData->renderQueue, taskData->renderTargetResource, 0));
	desc.depthStorage.loadOp = desc.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	desc.depthStorage.resource = shadowGrid->texture->GetRenderResource();
	desc.dimension = dim;

	render.UnmapResource(taskData->renderQueue, taskData->renderTargetResource, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(taskData->renderQueue, taskData->stateResource);
	render.ExecuteResource(taskData->renderQueue, taskData->renderTargetResource);

	CollectComponentsFromEntity(engine, *taskData, instanceData, captureData, shadowContext->rootEntity());
}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	// if unloading grid is still updating, we must build a new one
	if ((tiny->Flag().load(std::memory_order_acquire) & TINY_UPDATING) || tiny == currentGrid) {
		return nullptr;
	} else {
		// otherwise mark the current one dirty, and let load handler reuse.
		tiny->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
		return tiny;
	}
}

const Float3& LightComponent::GetColor() const {
	return color;
}

void LightComponent::SetColor(const Float3& c) {
	color = c;
}

float LightComponent::GetAttenuation() const {
	return attenuation;
}

void LightComponent::SetAttenuation(float value) {
	attenuation = value;
}

const Float3& LightComponent::GetRange() const {
	return range;
}

void LightComponent::SetRange(const Float3& r) {
	assert(r.x() > 0 && r.y() > 0 && r.z() > 0);
	range = r;
}

void LightComponent::ShadowLayer::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
	IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
	std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
	if (renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_DEFAULT) == ~(uint32_t)0) {
		taskData.Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed); // failed!!
		return;
	}

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
			group.animationComponent = instanceData.animationComponent();
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

LightComponentConfig::InstanceGroup::InstanceGroup() : instanceCount(0) {}

void LightComponentConfig::InstanceGroup::Cleanup() {
	instanceCount = 0;
	for (size_t k = 0; k < instancedData.size(); k++) {
		instancedData[k].Clear();
	}
}

LightComponentConfig::MergedInstanceGroup::MergedInstanceGroup() : groupPrototype(nullptr), mergedInstanceCount(0) {}

void LightComponentConfig::TaskData::RenderFrame(Engine& engine) {
	OPTICK_EVENT();

	assert(Flag().load(std::memory_order_acquire) & (TINY_MODIFIED | TINY_ACTIVATED));
	// engine.mythForest.StartCaptureFrame("lightdebug", "");
	std::vector<IRender::Queue*> renderQueues;
	renderQueues.emplace_back(renderQueue);
	engine.interfaces.render.SubmitQueues(&renderQueues[0], verify_cast<uint32_t>(renderQueues.size()), IRender::SUBMIT_EXECUTE_ALL);
	shadowGrid->Flag().fetch_and(~(TINY_MODIFIED | TINY_UPDATING), std::memory_order_release);
	shadowGrid = nullptr;
	Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	// engine.mythForest.EndCaptureFrame();
}

void LightComponent::ShadowLayer::CompleteCollect(Engine& engine, TaskData& taskData) {
	OPTICK_EVENT();
	taskData.rootEntity = nullptr;

	// collect success?
	bool activated = taskData.Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED;
	if (activated) {
		// assemble 
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

					assert(bufferRange.buffer == nullptr);
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

			ShaderResource* shaderResource = group.shaderResource();
			// skinning
			if (group.animationComponent) {
				const PassBase::Parameter& parameter = shaderResource->GetPassUpdater()[IShader::BindInput::BONE_TRANSFORMS];
				if (parameter) {
					size_t k = parameter.slot;
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.GetBuffers()[k];
					bufferRange.buffer = group.animationComponent->AcquireBoneMatrixBuffer(render, taskData.renderQueue);
					bufferRange.offset = 0;
					bufferRange.length = 0;
					bufferRange.component = 0;
					bufferRange.type = 0;
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
		engine.snowyStream.GetRenderResourceManager()->QueueFrameRoutine(CreateTaskContextFree(Wrap(&taskData, &TaskData::RenderFrame), std::ref(engine)), &taskData);
	}

	// Cleanup
	taskData.globalBytesCache.Reset();
	// failed! do cleanup immediately
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

	if (!activated) {
		// remain modified flag
		taskData.shadowGrid->Flag().fetch_and(~TINY_UPDATING, std::memory_order_release);
		taskData.shadowGrid = nullptr;
		taskData.Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}

void LightComponent::ShadowLayer::CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_relaxed);
	uint32_t warpIndex = entity->GetWarpIndex();
	assert(warpIndex == engine.GetKernel().GetCurrentWarpIndex());
	TaskData::WarpData& warpData = taskData.warpData[warpIndex];

	WorldInstanceData subWorldInstancedData = instanceData;
	MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();

	// has TransformComponent?
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	bool visible = true;
	if (transformComponent != nullptr) {
		localTransform = transformComponent->GetTransform();

		// IsVisible through visibility checking?
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if ((!(transformComponent->Flag().load(std::memory_order_relaxed) & TransformComponent::TRANSFORMCOMPONENT_DYNAMIC) && !VisibilityComponent::IsVisible(captureData.visData, transformComponent)) || !captureData(localBoundingBox)) {
			visible = false;
		}

		subWorldInstancedData.worldMatrix = localTransform * instanceData.worldMatrix;
	}

	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL | Entity::ENTITY_HAS_SPACE)) {
		// optional animation
		subWorldInstancedData.animationComponent = entity->GetUniqueComponent(UniqueType<AnimationComponent>());

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

			if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) continue;
			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();
			// if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) continue;

			if (entityMask & Entity::ENTITY_HAS_RENDERABLE) {
				if (visible) {
					assert(component->QueryInterface(UniqueType<RenderableComponent>()) != nullptr);
					RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
					if (!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
						if (renderableComponent->GetVisible()) {
							CollectRenderableComponent(engine, taskData, renderableComponent, warpData, subWorldInstancedData);
						}
					}
				}
			} else if (entityMask & Entity::ENTITY_HAS_SPACE) {
				assert(component->QueryInterface(UniqueType<SpaceComponent>()) != nullptr);
				WorldInstanceData subSpaceWorldInstancedData = subWorldInstancedData;
				subSpaceWorldInstancedData.animationComponent = nullptr; // animation info cannot be derived

				VisibilityComponent* visibilityComponent = entity->GetUniqueComponent(UniqueType<VisibilityComponent>());

				taskData.pendingCount.fetch_add(1, std::memory_order_relaxed);

				CaptureData newCaptureData;
				const MatrixFloat4x4& mat = captureData.viewTransform;
				const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(engine, Float3(mat(3, 0), mat(3, 1), mat(3, 2))) : Bytes::Null();
				newCaptureData.visData = visData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				if (transformComponent != nullptr) {
					OrthoCamera::UpdateCaptureData(newCaptureData, mat * Math::QuickInverse(localTransform));
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, newCaptureData, spaceComponent);
				} else {
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, captureData, spaceComponent);
				}
			}
		}
	}
}

LightComponentConfig::TaskData::TaskData(Engine& engine, uint32_t warpCount, const UShort2& resolution) : pendingCount(0) {
	warpData.resize(warpCount);
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();

	renderQueue = render.CreateQueue(device);

	stateResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
	IRender::Resource::RenderStateDescription& rs = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(renderQueue, stateResource, 0));
	rs.stencilReplacePass = 1;
	rs.cull = 1;
	rs.cullFrontFace = 1;
	rs.fill = 1;
	rs.blend = 0;
	rs.colorWrite = 0;
	rs.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	rs.depthWrite = 1;
	rs.stencilTest = 0;
	rs.stencilWrite = 0;
	rs.stencilValue = 0;
	rs.stencilMask = 0;
	render.UnmapResource(renderQueue, stateResource, IRender::MAP_DATA_EXCHANGE);

	renderTargetResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
	render.SetResourceNote(renderTargetResource, "ShadowMapBakePass");
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_release);
}

void LightComponentConfig::TaskData::Destroy(IRender& render) {
	shadowGrid = nullptr;

	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, renderTargetResource);
	render.DeleteQueue(renderQueue);
}

TObject<IReflect>& LightComponentConfig::TaskData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {}

	return *this;
}

TObject<IReflect>& LightComponentConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}
void LightComponent::ShadowLayer::Initialize(Engine& engine, const TShared<StreamComponent>& component, const UShort2& res, float size, float s) {
	OPTICK_EVENT();
	Uninitialize(engine);

	streamComponent = component;
	resolution = res;
	gridSize = size;
	scale = s;

	if (streamComponent) {
		streamComponent->SetLoadHandler(Wrap(this, &ShadowLayer::StreamLoadHandler));
		streamComponent->SetRefreshHandler(Wrap(this, &ShadowLayer::StreamRefreshHandler));
		streamComponent->SetUnloadHandler(Wrap(this, &ShadowLayer::StreamUnloadHandler));
	}

	if (!pipeline) {
		String path = ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName();
		pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());
	}

	TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("LightShadowBakeDummy", this), false, ResourceBase::RESOURCE_VIRTUAL);
	texture->description.dimension = UShort3(res.x(), res.y(), 1);
	texture->description.state.attachment = true;
	texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	texture->description.state.layout = IRender::Resource::TextureDescription::R;
	texture->description.state.addressU = texture->description.state.addressV = texture->description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
	texture->description.state.sample = IRender::Resource::TextureDescription::POINT;
	texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
	texture->GetResourceManager().InvokeUpload(texture(), engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue());

	dummyColorAttachment = texture;

	if (!currentTask) {
		currentTask = TShared<TaskData>::From(new TaskData(engine, engine.GetKernel().GetWarpCount(), res));
	}
}

void LightComponent::ShadowLayer::Uninitialize(Engine& engine) {
	OPTICK_EVENT();
	if (streamComponent) {
		streamComponent->SetLoadHandler(nullptr);
		streamComponent->SetRefreshHandler(nullptr);
		streamComponent->SetUnloadHandler(nullptr);
	}

	if (currentTask) {
		currentTask->Destroy(engine.interfaces.render);
		currentTask = nullptr;
	}
}

void LightComponent::ShadowLayer::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	OPTICK_EVENT();
	// compute grid id
	Float3 position(cameraTransform(3, 0), cameraTransform(3, 1), cameraTransform(3, 2));

	// project to ortho plane
	Float3 lightCoord = Math::Transform(Math::QuickInverse(lightTransform), position);
	const UShort3& dimension = streamComponent->GetDimension();

	float gridScaledSize = gridSize * scale;

	Int3 intPosition((int32_t)(lightCoord.x() / gridScaledSize), (int32_t)(lightCoord.y() / gridScaledSize), (int32_t)(lightCoord.z() / gridScaledSize));
	UShort3 coord = streamComponent->ComputeWrapCoordinate(intPosition);

	TShared<ShadowContext> shadowContext = TShared<ShadowContext>::From(new ShadowContext());
	shadowContext->rootEntity = rootEntity;
	shadowContext->lightTransformMatrix = Math::MatrixScale(Float4(scale, scale, scale, 1)) * lightTransform;

	// Make alignment
	Float3 alignedPosition = Math::Transform(lightTransform, Float3(intPosition.x() * gridScaledSize, intPosition.y() * gridScaledSize, intPosition.z() * gridScaledSize));
	shadowContext->lightTransformMatrix(3, 0) = alignedPosition.x();
	shadowContext->lightTransformMatrix(3, 1) = alignedPosition.y();
	shadowContext->lightTransformMatrix(3, 2) = alignedPosition.z();
	TShared<ShadowGrid> grid = streamComponent->Load(engine, coord, shadowContext())->QueryInterface(UniqueType<ShadowGrid>());
	assert(grid);

	if (!(grid->Flag().load(std::memory_order_relaxed) & TINY_MODIFIED) || !currentGrid) {
		/*
		if (currentGrid != grid) {
			engine.bridgeSunset.LogInfo().Printf("Update Shadow (%.3f, %.3f, %.3f) - (%d, %d, %d) => (%d, %d, %d) [Grid Size] = %.5f [Scale] = %.5f\n", position.x(), position.y(), position.z(), intPosition.x(), intPosition.y(), intPosition.z(), coord.x(), coord.y(), coord.z(), gridScaledSize, scale);
			engine.bridgeSunset.LogInfo().Printf("Aligned Position (%.3f, %.3f, %.3f)\n", alignedPosition.x(), alignedPosition.y(), alignedPosition.z());
		}*/

		currentGrid = grid;
	}
}

const TShared<LightComponent::ShadowGrid>& LightComponent::ShadowLayer::GetCurrent() const {
	return currentGrid;
}

