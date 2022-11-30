#include "CameraComponent.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../Animation/AnimationComponent.h"
#include "../Batch/BatchComponent.h"
#include "../Form/FormComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../EnvCube/EnvCubeComponent.h"
#include "../Light/LightComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../Phase/PhaseComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../RenderFlow/RenderPort/RenderPortRenderTarget.h"
#include "../RenderFlow/RenderPort/RenderPortLightSource.h"
#include "../RenderFlow/RenderPort/RenderPortCameraView.h"
#include "../RenderFlow/RenderPort/RenderPortCommandQueue.h"
#include "../../Engine.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <cmath>
#include <iterator>
#include <utility>

using namespace PaintsNow;

// From glu source code
const double PI = 3.14159265358979323846;
enum {
	// STENCIL_REFLECTION = 0x10,
	// STENCIL_BLOOM = 0x20,
	STENCIL_SHADOW = 0x40,
	STENCIL_LIGHTING = 0x80
};

CameraComponentConfig::TaskData::WarpData::WarpData() : entityCount(0), visibleEntityCount(0), triangleCount(0) {}

CameraComponent::CameraComponent(const TShared<RenderFlowComponent>& prenderFlowComponent, const String& name)
	: hostEntity(nullptr), collectedEntityCount(0), collectedVisibleEntityCount(0), collectedTriangleCount(0), collectedDrawCallCount(0), viewDistance(256), jitterScale(0.5f), jitterHistoryRatio(0.9f), jitterIndex(0), renderFlowComponent(std::move(prenderFlowComponent)), cameraViewPortName(name) {
	// Flag().fetch_or(CAMERACOMPONENT_PERSPECTIVE | CAMERACOMPONENT_UPDATE_COMMITTED | CAMERACOMPONENT_AGILE_RENDERING, std::memory_order_relaxed);
	Flag().fetch_or(CAMERACOMPONENT_PERSPECTIVE | CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_relaxed);
}

void CameraComponent::UpdateJitterMatrices(CameraComponentConfig::WorldGlobalData& worldGlobalData) {
	OPTICK_EVENT();
	if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_SUBPIXEL_JITTER) {
		jitterIndex = (jitterIndex + 1) % 9;

		MatrixFloat4x4 jitterMatrix = MatrixFloat4x4::Identity();
		UShort2 resolution = renderFlowComponent->GetMainResolution();
		static float jitterX[9] = { 1.0f / 2.0f, 1.0f / 4.0f, 3.0f / 4.0f, 1.0f / 8.0f, 5.0f / 8.0f, 3.0f / 8.0f, 7.0f / 8.0f, 1.0f / 16.0f, 9.0f / 16.0f };
		static float jitterY[9] = { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f / 9.0f, 4.0f / 9.0f, 7.0f / 9.0f, 2.0f / 9.0f, 5.0f / 9.0f, 8.0f / 9.0f, 1.0f / 27.0f };
		Float2 jitterOffset((jitterX[jitterIndex] - 0.5f) / Math::Max((int)resolution.x(), 1), (jitterY[jitterIndex] - 0.5f) / Math::Max((int)resolution.y(), 1));

		jitterOffset *= jitterScale;
		jitterMatrix(3, 0) += jitterOffset.x();
		jitterMatrix(3, 1) += jitterOffset.y();
		jitterIndex = (jitterIndex + 1) % 9;

		worldGlobalData.jitterMatrix = jitterMatrix;
		worldGlobalData.jitterOffset = jitterOffset;
		Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
	} else if (worldGlobalData.jitterOffset.x() != 0) {
		worldGlobalData.jitterMatrix = MatrixFloat4x4::Identity();
		worldGlobalData.jitterOffset = Float2(0, 0);

		Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
	}
	
}

float CameraComponent::GetViewDistance() const {
	return viewDistance;
}

void CameraComponent::SetViewDistance(float dist) {
	viewDistance = dist;
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

float CameraComponent::GetJitterScale() const {
	return jitterScale;
}

void CameraComponent::SetJitterScale(float scale) {
	jitterScale = scale;
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

float CameraComponent::GetJitterHistoryRatio() const {
	return jitterHistoryRatio;
}

void CameraComponent::SetJitterHistoryRatio(float ratio) {
	jitterHistoryRatio = ratio;
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void CameraComponent::GetPerspective(float& d, float& n, float& f, float& r) const {
	d = fov;
	n = nearPlane;
	f = farPlane;
	r = aspect;
}

void CameraComponent::SetPerspective(float d, float n, float f, float r) {
	fov = d;
	nearPlane = n;
	farPlane = f;
	aspect = r;
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void CameraComponent::UpdateRootMatrices(TaskData& taskData, const MatrixFloat4x4& cameraWorldMatrix) {
	MatrixFloat4x4 projectionMatrix = (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_PERSPECTIVE) ? Math::MatrixPerspective(fov, aspect, nearPlane, farPlane) : Math::MatrixOrtho(Float3(1, 1, 1));

	MatrixFloat4x4 viewMatrix = Math::QuickInverse(cameraWorldMatrix);
	taskData.worldGlobalData.inverseViewMatrix = cameraWorldMatrix;
	taskData.worldGlobalData.projectionMatrix = projectionMatrix;
	taskData.worldGlobalData.viewMatrix = viewMatrix;
	taskData.worldGlobalData.viewProjectionMatrix = viewMatrix * projectionMatrix;
	taskData.worldGlobalData.tanHalfFov = (float)tan(fov / 2.0f);
	taskData.worldGlobalData.viewPosition = Float3(cameraWorldMatrix(3, 0), cameraWorldMatrix(3, 1), cameraWorldMatrix(3, 2));
}

RenderFlowComponent* CameraComponent::GetRenderFlowComponent() const {
	return renderFlowComponent();
}

TShared<CameraComponentConfig::TaskData> CameraComponent::GetTaskData() {
	return prevTaskData;
}

Tiny::FLAG CameraComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT;
}

void CameraComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();

	BaseClass::Initialize(engine, entity);
	hostEntity = entity;

	RenderPort* port = renderFlowComponent->BeginPort(cameraViewPortName);
	if (port != nullptr) {
		RenderPortCameraView* cameraViewPort = port->QueryInterface(UniqueType<RenderPortCameraView>());
		if (cameraViewPort != nullptr) {
			cameraViewPort->eventTickHooks += Wrap(this, &CameraComponent::OnTickCameraViewPort);
		}

		renderFlowComponent->EndPort(port);
	}

	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	uint32_t warpCount = engine.bridgeSunset.GetKernel().GetWarpCount();
	IRender& render = engine.interfaces.render;

	prevTaskData = TShared<TaskData>::From(new TaskData(warpCount));
	nextTaskData = TShared<TaskData>::From(new TaskData(warpCount));
}

class CookieTransitionCleaner {
public:
	CookieTransitionCleaner(void* k = nullptr) : key(k) {}

	bool operator () (Entity* entity) const {
		FormComponent* formComponent = entity->GetUniqueComponent(UniqueType<FormComponent>());
		if (formComponent != nullptr) {
			formComponent->SetCookie(key, nullptr);
		}

		return true;
	}

	void* key;
};

void CameraComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();

	if (bridgeComponent) {
		CookieTransitionCleaner cookieCleaner(this);
		Entity* bridgeEntity = bridgeComponent->GetHostEntity();
		if (bridgeEntity != nullptr) {
			SpaceComponent::ForAllEntities(engine, bridgeComponent->GetHostEntity(), cookieCleaner);
		}

		bridgeComponent->Clear(engine);
		bridgeComponent = nullptr;
	}

	if (Flag().load(std::memory_order_acquire) & CAMERACOMPONENT_UPDATE_COLLECTING) {
		Kernel& kernel = engine.GetKernel();
		ThreadPool& threadPool = kernel.GetThreadPool();
		if (threadPool.GetThreadCount() != 0) {
			kernel.Wait(Flag(), CAMERACOMPONENT_UPDATE_COLLECTING, 0);
		}
	}

	RenderPort* port = renderFlowComponent->BeginPort(cameraViewPortName);
	if (port != nullptr) {
		RenderPortCameraView* cameraViewPort = port->QueryInterface(UniqueType<RenderPortCameraView>());
		if (cameraViewPort != nullptr) {
			cameraViewPort->eventTickHooks -= Wrap(this, &CameraComponent::OnTickCameraViewPort);
		}

		renderFlowComponent->EndPort(port);
	}

	IRender& render = engine.interfaces.render;
	prevTaskData->Destroy(render);
	prevTaskData = nullptr;
	nextTaskData->Destroy(render);
	nextTaskData = nullptr;
	hostEntity = nullptr;

	BaseClass::Uninitialize(engine, entity);
}

// Event Dispatcher
void CameraComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (event.eventID == Event::EVENT_TICK) {
		OnTickHost(event.engine, entity);
	}
}

void CameraComponent::Refresh(Engine& engine) {
	// force refresh transform
	TransformComponent* transformComponent = hostEntity->GetUniqueComponent(UniqueType<TransformComponent>());

	if (transformComponent != nullptr) {
		UpdateRootMatrices(*nextTaskData, transformComponent->GetTransform());
		Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
	}
}

void CameraComponent::Instancing(Engine& engine, TaskData& taskData) {
	OPTICK_EVENT();
	// instancing
	// do not sort at this moment.
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	IRender& render = engine.interfaces.render;
	uint32_t entityCount = 0;
	uint32_t visibleEntityCount = 0;
	uint32_t triangleCount = 0;
	uint32_t drawCallCount = 0;

	// Merge all instance group
	assert(taskData.mergedInstancedGroupMap.empty());
	for (size_t m = 0; m < taskData.warpData.size(); m++) {
		TaskData::WarpData& warpData = taskData.warpData[m];
		TaskData::WarpData::InstanceGroupMap& instanceGroup = warpData.instanceGroups;
		for (TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroup.begin(); it != instanceGroup.end(); ++it) {
			InstanceKey key = (*it).first;
			InstanceGroup& group = (*it).second;
			if (group.instanceCount == 0)
				continue;

			assert(group.drawCallDescription.GetShader() != nullptr);
			MergedInstanceGroup& merged = taskData.mergedInstancedGroupMap[key];
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

		entityCount += warpData.entityCount;
		visibleEntityCount += warpData.visibleEntityCount;
		triangleCount += warpData.triangleCount;
	}

	// Generate all render resources.
	for (TaskData::MergedInstanceGroupMap::iterator it = taskData.mergedInstancedGroupMap.begin(); it != taskData.mergedInstancedGroupMap.end(); ++it) {
		InstanceGroup& group = *(*it).second.groupPrototype;
		assert(group.instanceCount != 0);
		assert(group.drawCallDescription.GetShader() != nullptr);

		// Get or create instance buffer and render queue for RenderPolicy
		RenderPolicy* renderPolicy = group.renderPolicy();
		std::vector<KeyValue<RenderPolicy*, TaskData::PolicyData> >::iterator ip = BinaryFind(taskData.renderPolicyMap.begin(), taskData.renderPolicyMap.end(), renderPolicy);
		if (ip == taskData.renderPolicyMap.end()) {
			ip = BinaryInsert(taskData.renderPolicyMap, renderPolicy);
			(*ip).second.instanceBuffer = render.CreateResource(device, IRender::Resource::RESOURCE_BUFFER);
			(*ip).second.portQueue = render.CreateQueue(device, IRender::QUEUE_SECONDARY);
		}

		TaskData::PolicyData& policyData = ip->second;
		ShaderResource* shaderResource = group.shaderResource();

		// Generate buffer for global data
		PassBase::Updater& updater = shaderResource->GetPassUpdater();
		std::vector<KeyValue<ShaderResource*, TaskData::GlobalBufferItem> >::iterator ig = BinaryFind(policyData.worldGlobalBufferMap.begin(), policyData.worldGlobalBufferMap.end(), shaderResource);

		if (ig == policyData.worldGlobalBufferMap.end()) {
			ig = BinaryInsert(policyData.worldGlobalBufferMap, shaderResource);
			taskData.worldGlobalData.Export(ig->second.globalUpdater, updater);

			std::vector<Bytes, TCacheAllocator<Bytes> > s(&taskData.globalBytesCache);
			std::vector<IRender::Resource::DrawCallDescription::BufferRange, TCacheAllocator<IRender::Resource::DrawCallDescription::BufferRange> > bufferResources(&taskData.globalBytesCache);
			std::vector<IRender::Resource*, TCacheAllocator<IRender::Resource*> > textureResources(&taskData.globalBytesCache);
			ig->second.globalUpdater.Snapshot(s, bufferResources, textureResources, taskData.worldGlobalData, &taskData.globalBytesCache);
			IRender::Resource::DrawCallDescription::BufferRange range;
			memset(&range, 0, sizeof(range));
			ig->second.buffers.resize(updater.GetBufferCount(), range);

			for (size_t i = 0; i < s.size(); i++) {
				Bytes& data = s[i];
				if (!data.Empty()) {
					IRender::Resource* res = render.CreateResource(device, IRender::Resource::RESOURCE_BUFFER);
					// Do not upload here!
					/*
					IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(policyData.portQueue, res, 0));
					desc.usage = IRender::Resource::BufferDescription::UNIFORM;
					desc.component = 4;
					desc.dynamic = 1;
					desc.format = IRender::Resource::BufferDescription::FLOAT;
					desc.data = std::move(data);
					render.UnmapResource(policyData.portQueue, res, IRender::MAP_DATA_EXCHANGE);
					*/

					ig->second.buffers[i].buffer = res;
					policyData.runtimeResources.emplace_back(res);
				}
			}

			// fill global textures / buffers
			if (!textureResources.empty()) {
				ig->second.textures.resize(updater.GetTextureCount());
				for (size_t j = 0; j < textureResources.size(); j++) {
					IRender::Resource* res = textureResources[j];
					if (res != nullptr) {
						ig->second.textures[j] = res;
					}
				}
			}

			for (size_t k = 0; k < bufferResources.size(); k++) {
				IRender::Resource::DrawCallDescription::BufferRange& range = bufferResources[k];
				if (range.buffer != nullptr) {
					ig->second.buffers[k] = range;
				}
			}
		}

		// Fill group textures / buffers
		if (!ig->second.textures.empty()) {
			IRender::Resource** textures = group.drawCallDescription.GetTextures();
			for (size_t m = 0; m < group.drawCallDescription.textureCount; m++) {
				IRender::Resource*& texture = textures[m];
				if (ig->second.textures[m] != nullptr) {
					texture = ig->second.textures[m];
				}
			}
		}

		IRender::Resource::DrawCallDescription::BufferRange* bufferRanges = group.drawCallDescription.GetBuffers();
		for (size_t n = 0; n < group.drawCallDescription.bufferCount; n++) {
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = bufferRanges[n];
			if (ig->second.buffers[n].buffer != nullptr) {
				bufferRange = ig->second.buffers[n];
			}
		}

		// Setup skinning buffer
		if (group.animationComponent) {
			const PassBase::Parameter& parameter = updater[IShader::BindInput::BONE_TRANSFORMS];
			if (parameter) {
				size_t k = parameter.slot;
				IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.GetBuffers()[k];
				bufferRange.buffer = group.animationComponent->AcquireBoneMatrixBuffer(render, policyData.portQueue);
				bufferRange.offset = 0;
				bufferRange.length = 0;
				bufferRange.component = 0;
				bufferRange.type = 0;
			}
		}

		// Concat instanced data
		std::vector<Bytes>& mergedInstanceData = (*it).second.mergedInstanceData;
		uint32_t mergedInstanceCount = (*it).second.mergedInstanceCount;
		for (size_t k = 0; k < mergedInstanceData.size(); k++) {
			Bytes& data = mergedInstanceData[k];
			if (!data.Empty()) {
				// assign instanced buffer	
				size_t viewSize = data.GetViewSize();
				IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.GetBuffers()[k];

				assert(bufferRange.buffer == nullptr);
				bufferRange.buffer = policyData.instanceBuffer;
				bufferRange.offset = policyData.instanceOffset; // TODO: alignment
				bufferRange.length = 0;
				bufferRange.component = verify_cast<uint16_t>(viewSize / (mergedInstanceCount * sizeof(float)));
				bufferRange.type = 0;
				taskData.globalBytesCache.Link(policyData.instanceData, data);
				policyData.instanceOffset += verify_cast<uint32_t>(viewSize);
				assert(policyData.instanceOffset == policyData.instanceData.GetViewSize());
			}
		}

		group.drawCallDescription.instanceCount = verify_cast<uint16_t>(mergedInstanceCount);

		// Generate RenderState
		std::vector<KeyValue<IRender::Resource::RenderStateDescription, IRender::Resource*> >::iterator is = BinaryFind(policyData.renderStateMap.begin(), policyData.renderStateMap.end(), group.renderStateDescription);
		if (is == policyData.renderStateMap.end()) {
			is = BinaryInsert(policyData.renderStateMap, group.renderStateDescription);

			IRender::Resource*& state = is->second;
			state = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
			IRender::Resource::RenderStateDescription& desc = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(policyData.portQueue, state, 0));
			desc = group.renderStateDescription;
			render.UnmapResource(policyData.portQueue, state, IRender::MAP_DATA_EXCHANGE);
			// policyData.runtimeResources.emplace_back(state); // destroy on exit
		}

		group.renderStateResource = is->second;

		// Generate DrawCall
		assert(PassBase::ValidateDrawCall(group.drawCallDescription));

		IRender::Resource*& drawCall = group.drawCallResource;
		drawCall = render.CreateResource(device, IRender::Resource::RESOURCE_QUICK_DRAWCALL, policyData.portQueue);
		IRender::Resource::QuickDrawCallDescription& deviceDrawCall = *static_cast<IRender::Resource::QuickDrawCallDescription*>(render.MapResource(policyData.portQueue, drawCall, 0));
		deviceDrawCall = group.drawCallDescription;
		render.UnmapResource(policyData.portQueue, drawCall, IRender::MAP_DATA_EXCHANGE);
		policyData.runtimeResources.emplace_back(drawCall);
		drawCallCount++;
	}
	
	for (size_t n = 0; n < taskData.renderPolicyMap.size(); n++) {
		TaskData::PolicyData& policyData = taskData.renderPolicyMap[n].second;
		if (!policyData.instanceData.Empty()) {
			IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(policyData.portQueue, policyData.instanceBuffer, 0));
			desc.data.Resize(policyData.instanceOffset);
			desc.data.Import(0, policyData.instanceData);
			assert(policyData.instanceOffset == desc.data.GetSize());
			desc.state.format = IRender::Resource::BufferDescription::FLOAT;
			desc.state.usage = IRender::Resource::BufferDescription::INSTANCED;
			desc.state.dynamic = 1;
			desc.state.component = 0; // will be overridden by drawcall.
			render.UnmapResource(policyData.portQueue, policyData.instanceBuffer, IRender::MAP_DATA_EXCHANGE);
		}
	}

	// Update stats
	collectedEntityCount = entityCount;
	collectedVisibleEntityCount = visibleEntityCount;
	collectedTriangleCount = triangleCount;
	collectedDrawCallCount = drawCallCount;
}

void CameraComponent::UpdateTaskData(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	// Next collection ready? 
	if (!(Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_relaxed) & CAMERACOMPONENT_UPDATE_COMMITTED)) {
		return;
	}

	Flag().fetch_or(CAMERACOMPONENT_UPDATE_COLLECTING, std::memory_order_acq_rel);
	// Tick new collection
	nextTaskData->Cleanup(engine.interfaces.render);

	std::atomic_thread_fence(std::memory_order_acquire);

	WorldInstanceData worldInstanceData;
	worldInstanceData.worldMatrix = MatrixFloat4x4::Identity();

	CaptureData captureData;
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();
	if (transformComponent != nullptr) {
		localTransform = transformComponent->GetTransform();
	}

	UpdateRootMatrices(*nextTaskData, localTransform);

	nextTaskData->worldGlobalData.lastViewProjectionMatrix = prevTaskData->worldGlobalData.lastViewProjectionMatrix;

	UpdateCaptureData(captureData, localTransform);

	// Must called from entity thread
	Entity* rootEntity = bridgeComponent->GetHostEntity();
	assert(engine.GetKernel().GetCurrentWarpIndex() == rootEntity->GetWarpIndex());
	VisibilityComponent* visibilityComponent = rootEntity->GetUniqueComponent(UniqueType<VisibilityComponent>());
	const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(engine, Float3(localTransform(3, 0), localTransform(3, 1), localTransform(3, 2))) : Bytes::Null();

	captureData.visData = visData;
	CollectComponentsFromEntity(engine, *nextTaskData, worldInstanceData, captureData, rootEntity);
}

template <class T>
T* QueryPort(Engine& engine, std::map<RenderPolicy*, RenderStage::Port*>& policyPortMap, RenderFlowComponent* renderFlowComponent, RenderPolicy* lastRenderPolicy, UniqueType<T>) {
	std::map<RenderPolicy*, RenderStage::Port*>::iterator it = policyPortMap.find(lastRenderPolicy);
	if (it != policyPortMap.end()) return it->second == nullptr ? nullptr : it->second->QueryInterface(UniqueType<T>());

	RenderStage::Port*& port = policyPortMap[lastRenderPolicy];
	port = renderFlowComponent->BeginPort(lastRenderPolicy->renderPortName);

	if (port != nullptr) {
		T* t = port->QueryInterface(UniqueType<T>());
		if (t != nullptr) {
			t->OnFrameEncodeBegin(engine);
			return t;
		}
	}

	return nullptr;
}

void CameraComponent::CommitRenderRequests(Engine& engine, TaskData& taskData, IRender::Queue* queue) {
	OPTICK_EVENT();
	// commit to RenderFlowComponent
	// update data updaters
	if (renderFlowComponent) {
		IRender& render = engine.interfaces.render;
		uint32_t updateCount = 0;
		std::vector<TaskData::WarpData>& warpData = taskData.warpData;
	
		std::vector<IDataUpdater*> updaters;
		for (size_t i = 0; i < warpData.size(); i++) {
			TaskData::WarpData& w = warpData[i];
			for (size_t k = 0; k < w.dataUpdaters.size(); k++) {
				BinaryInsert(updaters, w.dataUpdaters[k]);
			}
		}

		for (size_t k = 0; k < updaters.size(); k++) {
			updateCount += updaters[k]->Update(render, queue);
		}

		std::map<RenderPolicy*, RenderStage::Port*> policyPortMap;
		const Float3& viewPosition = taskData.worldGlobalData.viewPosition;

		TShared<TextureResource> cubeMapTexture;
		TShared<TextureResource> skyMapTexture;
		float minDist = FLT_MAX;

		// merge RenderPolicy data
		RenderPolicy* lastRenderPolicy = nullptr;
		RenderPortCommandQueue* lastCommandQueue = nullptr;
		IRender::Resource* lastRenderState = nullptr;

		for (TaskData::MergedInstanceGroupMap::iterator it = taskData.mergedInstancedGroupMap.begin(); it != taskData.mergedInstancedGroupMap.end(); ++it) {
			MergedInstanceGroup& mergedGroup = (*it).second;
			InstanceGroup& group = *mergedGroup.groupPrototype;
			assert(mergedGroup.mergedInstanceCount != 0);

			if (group.renderPolicy != lastRenderPolicy) {
				lastRenderPolicy = group.renderPolicy();
				lastRenderState = nullptr;
				lastCommandQueue = lastRenderPolicy == nullptr ? nullptr : QueryPort(engine, policyPortMap, renderFlowComponent(), lastRenderPolicy, UniqueType<RenderPortCommandQueue>());
				if (lastCommandQueue != nullptr) {
					std::vector<KeyValue<RenderPolicy*, TaskData::PolicyData> >::iterator ip = BinaryFind(taskData.renderPolicyMap.begin(), taskData.renderPolicyMap.end(), lastRenderPolicy);
					if (ip != taskData.renderPolicyMap.end()) {
						TaskData::PolicyData& policyData = ip->second;
						// assert(policyData.instanceBuffer != nullptr);
						// policyData.instanceBuffer = (IRender::Resource*)((size_t)policyData.instanceBuffer | 1);
						render.FlushQueue(policyData.portQueue);
						lastCommandQueue->MergeQueue(render, policyData.portQueue, true);
					}
				}
			}

			IRender::Resource* drawCallResource = group.drawCallResource;
			if (lastCommandQueue != nullptr && drawCallResource != nullptr) {
				IRender::Resource* renderState = group.renderStateResource;
				if (renderState != lastRenderState) {
					lastCommandQueue->CheckinState(render, renderState);
					lastRenderState = renderState;
				}

				lastCommandQueue->DrawElement(render, drawCallResource);
			}
		}

		for (size_t m = 0; m < taskData.warpData.size(); m++) {
			TaskData::WarpData& warpData = taskData.warpData[m];
			// pass lights
			std::vector<std::pair<RenderPolicy*, LightElement> >& lightElements = warpData.lightElements;
			for (size_t u = 0; u < lightElements.size(); u++) {
				std::pair<RenderPolicy*, LightElement>& e = lightElements[u];
				LightElement& element = e.second;
				RenderPolicy* renderPolicy = e.first;
				if (renderPolicy != nullptr) {
					RenderPortLightSource* portLightSource = QueryPort(engine, policyPortMap, renderFlowComponent(), renderPolicy, UniqueType<RenderPortLightSource>());
					if (portLightSource != nullptr) {
						if (portLightSource->lightElements.empty()) {
							portLightSource->stencilMask = STENCIL_LIGHTING;
						}

						portLightSource->stencilShadow = STENCIL_SHADOW;
						portLightSource->lightElements.emplace_back(std::move(element));

						if (element.position.w() == 0) {
							for (size_t i = 0; i < portLightSource->lightElements.size() - 1; i++) {
								LightElement& elem = portLightSource->lightElements[i];
								if (elem.position.w() != 0) {
									std::swap(portLightSource->lightElements.back(), elem);
									break;
								}
							}
						}
					}
				}
			}

			for (size_t j = 0; j < warpData.envCubeElements.size(); j++) {
				std::pair<RenderPolicy*, EnvCubeElement>& e = warpData.envCubeElements[j];
				EnvCubeElement& element = e.second;
				RenderPolicy* renderPolicy = e.first;
				if (renderPolicy != nullptr) {
					RenderPortLightSource* portLightSource = QueryPort(engine, policyPortMap, renderFlowComponent(), renderPolicy, UniqueType<RenderPortLightSource>());
					if (portLightSource != nullptr) {
						float dist = Math::SquareLength(element.position - viewPosition);
						if (dist < minDist) {
							portLightSource->cubeMapTexture = element.cubeMapTexture ? element.cubeMapTexture : portLightSource->cubeMapTexture;
							portLightSource->skyMapTexture = element.skyMapTexture ? element.skyMapTexture : portLightSource->skyMapTexture;
							portLightSource->cubeStrength = element.cubeStrength;
							minDist = dist;
						}
					}
				}
			}
		}

		for (std::map<RenderPolicy*, RenderPort*>::iterator ip = policyPortMap.begin(); ip != policyPortMap.end(); ++ip) {
			if (ip->second != nullptr) {
				ip->second->OnFrameEncodeEnd(engine);
				renderFlowComponent->EndPort(ip->second);
			}
		}
	}
}

void CameraComponent::OnTickCameraViewPort(Engine& engine, RenderPort& renderPort, IRender::Queue* queue) {
	OPTICK_EVENT();
	std::atomic_thread_fence(std::memory_order_acquire);

	if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_UPDATE_COLLECTED) {
		if (nextTaskData->Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED) {
			CommitRenderRequests(engine, *nextTaskData, queue);
		}

		std::swap(prevTaskData, nextTaskData);
		Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COLLECTED, std::memory_order_relaxed);
		Flag().fetch_or(CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_acq_rel);
	}

	TShared<TaskData> taskData = prevTaskData;

	// Update jitter
	CameraComponentConfig::WorldGlobalData& worldGlobalData = taskData->worldGlobalData;
	std::vector<TaskData::WarpData>& warpData = taskData->warpData;
	IRender& render = engine.interfaces.render;

	// update camera view settings
	// TODO: fix shadow flickering when update CameraView settings in every frame, such as:
	// if (renderFlowComponent) {
	// here is a work-around:
	if (renderFlowComponent) {
		worldGlobalData.viewProjectionMatrix = worldGlobalData.viewMatrix * worldGlobalData.projectionMatrix;

		// CameraView settings
		RenderStage::Port* port = renderFlowComponent->BeginPort(cameraViewPortName);
		if (port != nullptr) {
			RenderPortCameraView* portCameraView = port->QueryInterface(UniqueType<RenderPortCameraView>());
			if (portCameraView != nullptr) {
				UpdateJitterMatrices(worldGlobalData);

				portCameraView->viewMatrix = worldGlobalData.viewMatrix;
				portCameraView->inverseViewMatrix = worldGlobalData.inverseViewMatrix;
				portCameraView->projectionMatrix = worldGlobalData.projectionMatrix * worldGlobalData.jitterMatrix;
				portCameraView->inverseProjectionMatrix = Math::InversePerspective(portCameraView->projectionMatrix); // it's jittered
				portCameraView->reprojectionMatrix = portCameraView->inverseProjectionMatrix * portCameraView->inverseViewMatrix * worldGlobalData.lastViewProjectionMatrix;
				portCameraView->projectionParams = Math::CompressPerspective(worldGlobalData.projectionMatrix);
				portCameraView->inverseProjectionParams = Math::CompressInversePerspective(worldGlobalData.projectionMatrix);
				portCameraView->jitterOffset = worldGlobalData.jitterOffset;
				portCameraView->jitterHistoryRatio = jitterHistoryRatio;

				worldGlobalData.viewProjectionMatrix = worldGlobalData.viewProjectionMatrix * worldGlobalData.jitterMatrix;
			}

			renderFlowComponent->EndPort(port);
		}

		// update buffers
		if (Flag().fetch_and(~TINY_MODIFIED, std::memory_order_relaxed) & TINY_MODIFIED) {
			OPTICK_PUSH("Updating render data");

			typedef std::vector<KeyValue<ShaderResource*, TaskData::GlobalBufferItem> > GlobalMap;
			for (std::vector<KeyValue<RenderPolicy*, TaskData::PolicyData> >::iterator it = taskData->renderPolicyMap.begin(); it != taskData->renderPolicyMap.end(); ++it) {
				GlobalMap& globalMap = it->second.worldGlobalBufferMap;

				for (GlobalMap::iterator it = globalMap.begin(); it != globalMap.end(); ++it) {
					std::vector<Bytes> buffers;
					std::vector<IRender::Resource*> textureResources;
					std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
					it->second.globalUpdater.Snapshot(buffers, bufferResources, textureResources, worldGlobalData, nullptr);

					assert(bufferResources.empty());
					assert(textureResources.empty());

					for (size_t i = 0; i < buffers.size(); i++) {
						Bytes& data = buffers[i];
						if (!data.Empty()) {
							IRender::Resource* buffer = it->second.buffers[i].buffer;
							IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
							desc.state.usage = IRender::Resource::BufferDescription::UNIFORM;
							desc.state.component = 4;
							desc.state.dynamic = 1;
							desc.state.format = IRender::Resource::BufferDescription::FLOAT;
							desc.data = std::move(data);
							render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);
						}
					}
				}
			}

			OPTICK_POP();
		}

		worldGlobalData.lastViewProjectionMatrix = worldGlobalData.viewMatrix * worldGlobalData.projectionMatrix;
	}

	renderPort.Flag().fetch_or(TINY_MODIFIED, std::memory_order_acq_rel);
}

void CameraComponent::OnTickHost(Engine& engine, Entity* entity) {
	OPTICK_EVENT();

	if (bridgeComponent) {
		assert(entity == hostEntity);
		Entity* bridgeEntity = bridgeComponent->GetHostEntity();
		if (bridgeEntity != nullptr && (bridgeEntity->Flag().load(std::memory_order_acquire) & Entity::ENTITY_HAS_SPACE)) {
			UpdateTaskData(engine, entity);
		}
	}
}

void CameraComponent::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData) {
	OPTICK_EVENT();

	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	IDrawCallProvider::InputRenderData inputRenderData(instanceData.viewReference, nullptr, renderFlowComponent->GetMainResolution());
	IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
	std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
	if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_AGILE_RENDERING) {
		renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_AGILE_RENDERING);
	} else {
		if (renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_DEFAULT) == ~(uint32_t)0) {
			taskData.Flag().fetch_and(~TINY_ACTIVATED);
			return;
		}
	}

	TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;
	bool isCameraViewSpace = !!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW);
	const std::vector<TShared<RenderPolicy> >& renderPolicies = renderableComponent->GetRenderPolicies();

	for (size_t j = 0; j < renderPolicies.size(); j++) {
		RenderPolicy* renderPolicy = renderPolicies[j]();

		for (size_t k = 0; k < drawCalls.size(); k++) {
			// PassBase& Pass = provider->GetPass(k);
			IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
			if (drawCall.priority < renderPolicy->priorityRange.first || drawCall.priority >= renderPolicy->priorityRange.second) {
				continue;
			}

			uint32_t div;
			switch (drawCall.drawCallDescription.GetIndexBuffer()->type) {
				case IRender::Resource::BufferDescription::Format::UNSIGNED_BYTE:
					div = 3;
					break;
				case IRender::Resource::BufferDescription::Format::UNSIGNED_SHORT:
					div = 6;
					break;
				default:
					div = 12;
					break;
			}

			warpData.triangleCount += drawCall.drawCallDescription.GetIndexBuffer()->length / div * Math::Max((uint32_t)1u, (uint32_t)drawCall.drawCallDescription.instanceCount);

			IRender::Resource::QuickDrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;

			// Add Lighting stencil
			assert(!(drawCall.renderStateDescription.stencilValue & STENCIL_LIGHTING));
			drawCall.renderStateDescription.stencilValue |= STENCIL_LIGHTING;
			drawCall.renderStateDescription.stencilMask |= STENCIL_LIGHTING;

			assert(drawCall.hashValue != 0);
			InstanceKey key = (InstanceKey)HashValue(renderPolicy, drawCall.hashValue);
			TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroups.find(key);
			std::vector<Bytes> s;
			std::vector<IRender::Resource*> textureResources;
			std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
			InstanceGroup& group = instanceGroups[key];

			if (group.instanceCount == 0) {
				BinaryInsert(warpData.dataUpdaters, drawCall.dataUpdater);
				group.renderPolicy = renderPolicy;
				group.shaderResource = drawCall.shaderResource;
				group.animationComponent = instanceData.animationComponent;
				instanceData.Export(group.instanceUpdater, drawCall.shaderResource->GetPassUpdater());

				// add renderstate if exists
				IRender::Resource::RenderStateDescription& renderStateDescription = drawCall.renderStateDescription;

				// apply renderPolicy renderstate modifier
				static_assert(sizeof(renderPolicy->renderStateMask) == sizeof(uint32_t), "Please use larger integer type here!");
				static_assert(sizeof(renderPolicy->renderStateTemplate) == sizeof(uint32_t), "Please use larger integer type here!");
				static_assert(sizeof(renderStateDescription) == sizeof(uint32_t), "Please use larger integer type here!");

				uint32_t renderStateMask = *reinterpret_cast<uint32_t*>(&renderPolicy->renderStateMask);
				uint32_t renderStateTemplate = *reinterpret_cast<uint32_t*>(&renderPolicy->renderStateTemplate);
				uint32_t& renderStateTarget = *reinterpret_cast<uint32_t*>(&renderStateDescription);
				renderStateTarget = (renderStateTarget & ~renderStateMask) | (renderStateTemplate | renderStateMask);
				group.renderStateDescription = renderStateDescription;
				group.drawCallDescription = drawCallTemplate;
#ifdef _DEBUG
				group.description = renderableComponent->GetDescription();
#endif // _DEBUG
			}

			// process local instanced data
			if (drawCall.localInstancedData.empty() && drawCall.localTransforms.empty()) {
				group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
				assert(!group.instanceUpdater.parameters.empty());
				group.instanceCount++;
			} else {
				assert(drawCall.drawCallDescription.instanceCount != 0);

				for (size_t i = 0; i < drawCall.localInstancedData.size(); i++) {
					std::pair<uint32_t, Bytes>& localInstancedData = drawCall.localInstancedData[i];
					if (group.instancedData.size() <= localInstancedData.first) {
						group.instancedData.resize(localInstancedData.first + 1);
					}

					if (!localInstancedData.second.IsViewStorage()) {
						Bytes bytes = warpData.bytesCache.New(verify_cast<uint32_t>(localInstancedData.second.GetSize()));
						bytes.Import(0, localInstancedData.second.GetData(), localInstancedData.second.GetSize());
						warpData.bytesCache.Link(group.instancedData[localInstancedData.first], bytes);
					} else {
						warpData.bytesCache.Link(group.instancedData[localInstancedData.first], localInstancedData.second);
					}
				}

				if (drawCall.localTransforms.empty()) {
					std::vector<Bytes> s;
					if (isCameraViewSpace) {
						WorldInstanceData subInstanceData = instanceData;
						subInstanceData.worldMatrix = instanceData.worldMatrix * taskData.worldGlobalData.viewMatrix;
						group.instanceUpdater.Snapshot(s, bufferResources, textureResources, subInstanceData, &warpData.bytesCache);
					} else {
						group.instanceUpdater.Snapshot(s, bufferResources, textureResources, instanceData, &warpData.bytesCache);
					}
					assert(drawCall.drawCallDescription.instanceCount != 0);
					for (size_t j = 0; j < s.size(); j++) {
						uint32_t viewSize = verify_cast<uint32_t>(s[j].GetViewSize());
						if (viewSize != 0) {
							Bytes view = warpData.bytesCache.New(viewSize * drawCall.drawCallDescription.instanceCount);
							view.Import(0, s[j], drawCall.drawCallDescription.instanceCount);
							warpData.bytesCache.Link(group.instancedData[j], view);
						}
					}
				} else {
					assert(drawCall.drawCallDescription.instanceCount == drawCall.localTransforms.size());
					WorldInstanceData subInstanceData = instanceData;
					MatrixFloat4x4 worldMatrix = isCameraViewSpace ? subInstanceData.worldMatrix * taskData.worldGlobalData.viewMatrix : instanceData.worldMatrix;

					for (size_t n = 0; n < drawCall.drawCallDescription.instanceCount; n++) {
						subInstanceData.worldMatrix = drawCall.localTransforms[n] * worldMatrix;
						group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, subInstanceData, &warpData.bytesCache);
					}
				}

				group.instanceCount += drawCall.drawCallDescription.instanceCount;
			}
		}
	}
}

void CameraComponent::CollectEnvCubeComponent(EnvCubeComponent* envCubeComponent, std::vector<std::pair<RenderPolicy*, EnvCubeElement> >& envCubeElements, const MatrixFloat4x4& worldMatrix) const {
	OPTICK_EVENT();
	EnvCubeElement element;
	element.position = Float3(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2));
	element.cubeMapTexture = envCubeComponent->cubeMapTexture;
	element.cubeStrength = envCubeComponent->strength;

	const std::vector<TShared<RenderPolicy> >& renderPolicies = envCubeComponent->GetRenderPolicies();
	for (size_t i = 0; i < renderPolicies.size(); i++) {
		envCubeElements.emplace_back(std::make_pair(renderPolicies[i](), std::move(element)));
	}
}

void CameraComponent::CompleteCollect(Engine& engine, TaskData& taskData) {
	OPTICK_EVENT();

	Kernel& kernel = engine.GetKernel();

	if (kernel.GetCurrentWarpIndex() != GetWarpIndex()) {
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &CameraComponent::CompleteCollect), std::ref(engine), std::ref(taskData)));
	} else {
		if (Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COLLECTING, std::memory_order_relaxed) & TINY_ACTIVATED) {
			if (nextTaskData->Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED) {
				Instancing(engine, taskData);
			}
		}

		Flag().fetch_or(CAMERACOMPONENT_UPDATE_COLLECTED | TINY_MODIFIED, std::memory_order_release);
	}
}

void CameraComponent::CollectLightComponent(Engine& engine, LightComponent* lightComponent, std::vector<std::pair<RenderPolicy*, LightElement> >& lightElements, const MatrixFloat4x4& worldMatrix, const TaskData& taskData) const {
	OPTICK_EVENT();
	LightElement element;
	Entity* rootEntity = bridgeComponent->GetHostEntity();
	if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
		element.position = Float4(-worldMatrix(2, 0), -worldMatrix(2, 1), -worldMatrix(2, 2), 0);

		// refresh shadow
		const std::vector<TShared<LightComponent::ShadowLayer> >& shadowLayers = lightComponent->UpdateShadow(engine, taskData.worldGlobalData.inverseViewMatrix, worldMatrix, rootEntity);

		element.shadows.reserve(shadowLayers.size());
		for (size_t i = 0; i < shadowLayers.size(); i++) {
			const TShared<LightComponent::ShadowGrid>& grid = shadowLayers[i]->GetCurrent();
			RenderPortLightSource::LightElement::Shadow shadow;
			shadow.shadowTexture = grid->Flag().load(std::memory_order_relaxed) & TINY_MODIFIED ? nullptr : grid->texture;
			shadow.shadowMatrix = grid->shadowMatrix;
			element.shadows.emplace_back(std::move(shadow));
		}
	} else {
		float range = lightComponent->GetRange().x();
		element.position = Float4(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2), Math::Max(0.05f, range * range));
	}

	const Float3& color = lightComponent->GetColor();
	element.colorAttenuation = Float4(color.x(), color.y(), color.z(), lightComponent->GetAttenuation());
	const std::vector<TShared<RenderPolicy> >& renderPolicies = lightComponent->GetRenderPolicies();

	for (size_t i = 0; i < renderPolicies.size(); i++) {
		lightElements.emplace_back(std::make_pair(renderPolicies[i](), std::move(element)));
	}
}

uint32_t CameraComponent::GetCollectedEntityCount() const {
	return collectedEntityCount;
}

uint32_t CameraComponent::GetCollectedVisibleEntityCount() const {
	return collectedVisibleEntityCount;
}

uint32_t CameraComponent::GetCollectedTriangleCount() const {
	return collectedTriangleCount;
}

uint32_t CameraComponent::GetCollectedDrawCallCount() const {
	return collectedDrawCallCount;
}

void CameraComponent::CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	if (transformComponent != nullptr) {
		IMemory::PrefetchReadLocal(transformComponent); // hacked hint for TransformComponent
		if /*constexpr*/ (sizeof(TransformComponent) > CPU_CACHELINE_SIZE) {
			IMemory::PrefetchReadLocal(reinterpret_cast<uint8_t*>(transformComponent) + CPU_CACHELINE_SIZE);
		}
	}

	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_relaxed);
	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL | Entity::ENTITY_HAS_SPACE)) {
		uint32_t warpIndex = entity->GetWarpIndex();
		assert(warpIndex == engine.GetKernel().GetCurrentWarpIndex());
		TaskData::WarpData& warpData = taskData.warpData[warpIndex];

		// optional animation
		WorldInstanceData subWorldInstancedData = instanceData;
		subWorldInstancedData.animationComponent = entity->GetUniqueComponent(UniqueType<AnimationComponent>());

		ExplorerComponent::ComponentPointerAllocator allocator(&warpData.bytesCache);
		std::vector<Component*, ExplorerComponent::ComponentPointerAllocator> exploredComponents(allocator);
		ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());

		singleton Unique expectedIdentifier = UniqueType<RenderableComponent>::Get();
		if (explorerComponent != nullptr && explorerComponent->GetExploreIdentifier() == expectedIdentifier) {
			// Use nearest refValue for selecting most detailed components
			explorerComponent->SelectComponents(engine, entity, subWorldInstancedData.viewReference, exploredComponents);
		} else {
			exploredComponents.reserve(entity->GetComponentCount());
			entity->CollectComponents(exploredComponents);
		}

		MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();

		bool visible = true;
		if (transformComponent != nullptr) {
			// IsVisible through visibility checking?
			const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
			if ((!(transformComponent->Flag().load(std::memory_order_relaxed) & TransformComponent::TRANSFORMCOMPONENT_DYNAMIC) && !VisibilityComponent::IsVisible(captureData.visData, transformComponent)) || !captureData(localBoundingBox)) {
				visible = false;
			}

			// Fetch screen ratio
			float tanHalfFov = taskData.worldGlobalData.tanHalfFov;

			Float4 start = Float4::Load(localBoundingBox.first) * instanceData.worldMatrix;
			Float4 end = Float4::Load(localBoundingBox.second) * instanceData.worldMatrix;
			Float4 center = (start + end) * Float4(0.5f, 0.5f, 0.5f, 0.0f);
			float length = Math::SquareLength(end - start);
			float refValue = 1.0f - length / (0.001f + tanHalfFov * Math::SquareLength(center - Float4::Load(captureData.GetPosition(), 0.0f)));

			// Project
			subWorldInstancedData.viewReference = Math::Max(0.0f, refValue);
			localTransform = transformComponent->GetTransform();
			subWorldInstancedData.worldMatrix = localTransform * instanceData.worldMatrix;
		}

		for (size_t k = 0; k < exploredComponents.size(); k++) {
			Component* component = exploredComponents[k];
			assert(component != nullptr);

			if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) {
				if (!(Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_AGILE_RENDERING)) {
					taskData.Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed); // fail!
					break;
				} else {
					continue;
				}
			}

			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();

			if (entityMask & Entity::ENTITY_HAS_RENDERABLE) {
				bool isRenderControl = !!(entityMask & Entity::ENTITY_HAS_RENDERCONTROL);
				if (visible) {
					assert(component->QueryInterface(UniqueType<RenderableComponent>()) != nullptr);
					RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
					if (renderableComponent->GetVisible()) {
						CollectRenderableComponent(engine, taskData, renderableComponent, warpData, subWorldInstancedData);
						if (!isRenderControl) {
							++warpData.visibleEntityCount;
						}
					}
				}

				if (isRenderControl) {
					LightComponent* lightComponent;
					EnvCubeComponent* envCubeComponent;
					if ((lightComponent = component->QueryInterface(UniqueType<LightComponent>())) != nullptr) {
						CollectLightComponent(engine, lightComponent, warpData.lightElements, subWorldInstancedData.worldMatrix, taskData);
					} else if ((envCubeComponent = component->QueryInterface(UniqueType<EnvCubeComponent>())) != nullptr) {
						CollectEnvCubeComponent(envCubeComponent, warpData.envCubeElements, subWorldInstancedData.worldMatrix);
					}
				} else {
					++warpData.entityCount;
				}
			} else if (entityMask & Entity::ENTITY_HAS_SPACE) {
				assert(component->QueryInterface(UniqueType<SpaceComponent>()) != nullptr);
				WorldInstanceData subSpaceWorldInstancedData = subWorldInstancedData;
				subSpaceWorldInstancedData.animationComponent = nullptr; // animation info cannot be derived

				VisibilityComponent* visibilityComponent = entity->GetUniqueComponent(UniqueType<VisibilityComponent>());
				EventComponent* eventComponent = entity->GetUniqueComponent(UniqueType<EventComponent>());
				taskData.pendingCount.fetch_add(1, std::memory_order_relaxed);

				CaptureData newCaptureData = captureData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				const MatrixFloat4x4& mat = captureData.viewTransform;

				if (transformComponent != nullptr) {
					UpdateCaptureData(newCaptureData, mat * Math::QuickInverse(localTransform));
				}

				const MatrixFloat4x4& newMat = newCaptureData.viewTransform;
				if (visibilityComponent != nullptr) {
					newCaptureData.visData = visibilityComponent->QuerySample(engine, Float3(newMat(3, 0), newMat(3, 1), newMat(3, 2)));
				}

				CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, newCaptureData, spaceComponent);
			}
		}
	}
}

const TShared<BridgeComponent>& CameraComponent::GetBridgeComponent() const {
	return bridgeComponent;
}

void CameraComponent::BindRootEntity(Engine& engine, BridgeComponentModule& bridgeComponentModule, Entity* entity) {
	assert(!bridgeComponent);
	assert(entity != nullptr);
	bridgeComponent = bridgeComponentModule.New(this);
	entity->AddComponent(engine, bridgeComponent());
}

CameraComponentConfig::TaskData::TaskData(uint32_t warpCount) 
#if !defined(_MSC_VER) || _MSC_VER > 1200
	: mergedInstancedGroupMap(&globalBytesCache)
#endif
{
	warpData.resize(warpCount);
	pendingCount.store(0, std::memory_order_relaxed);
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_release);
}

TObject<IReflect>& CameraComponentConfig::TaskData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldGlobalData);
	}

	return *this;
}

void CameraComponentConfig::TaskData::Cleanup(IRender& render) {
	OPTICK_EVENT();
	for (std::vector<KeyValue<RenderPolicy*, PolicyData> >::iterator it = renderPolicyMap.begin(); it != renderPolicyMap.end(); ++it) {
		PolicyData& policyData = it->second;
		IRender::Queue* queue = policyData.portQueue;

		for (size_t k = 0; k < policyData.runtimeResources.size(); k++) {
			render.DeleteResource(queue, policyData.runtimeResources[k]);
		}

		policyData.worldGlobalBufferMap.clear();
		policyData.instanceData.Clear();
		policyData.instanceOffset = 0;
		policyData.runtimeResources.clear();
	}

	for (size_t i = 0; i < warpData.size(); i++) {
		WarpData& data = warpData[i];

		data.bytesCache.Reset();
		data.entityCount = 0;
		data.dataUpdaters.clear();
		data.envCubeElements.clear();
		data.lightElements.clear();
		data.visibleEntityCount = 0;
		data.triangleCount = 0;

		// to avoid frequently memory alloc/dealloc
		// data.instanceGroups.clear();
		for (WarpData::InstanceGroupMap::iterator ip = data.instanceGroups.begin(); ip != data.instanceGroups.end();) {
			InstanceGroup& group = (*ip).second;

			if (group.instanceCount == 0) {
				// to be deleted.
				data.instanceGroups.erase(ip++);
			} else {
				group.Cleanup();
				++ip;
			}
		}
	}

	globalBytesCache.Reset();
#if defined(_MSC_VER) && _MSC_VER <= 1200
	mergedInstancedGroupMap.clear();
#else
	std::destruct(&mergedInstancedGroupMap);
	new (&mergedInstancedGroupMap) TaskData::MergedInstanceGroupMap(&globalBytesCache);
#endif

	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_release);
}

CameraComponentConfig::InstanceGroup::InstanceGroup() : instanceCount(0), drawCallResource(nullptr), renderStateResource(nullptr) {}

void CameraComponentConfig::InstanceGroup::Cleanup() {
	instanceCount = 0;
	for (size_t i = 0; i < instancedData.size(); i++) {
		instancedData[i].Clear();
	}
}

CameraComponentConfig::MergedInstanceGroup::MergedInstanceGroup() : groupPrototype(nullptr), mergedInstanceCount(0) {}

CameraComponentConfig::TaskData::~TaskData() {
	assert(warpData.empty());
}

CameraComponentConfig::TaskData::PolicyData::PolicyData() : portQueue(nullptr), instanceBuffer(nullptr), instanceOffset(0) {}

void CameraComponentConfig::TaskData::Destroy(IRender& render) {
	Cleanup(render);

	for (std::vector<KeyValue<RenderPolicy*, PolicyData> >::iterator it = renderPolicyMap.begin(); it != renderPolicyMap.end(); ++it) {
		PolicyData& policyData = it->second;

		for (size_t k = 0; k < policyData.renderStateMap.size(); k++) {
			render.DeleteResource(policyData.portQueue, policyData.renderStateMap[k].second);
		}

		render.DeleteResource(policyData.portQueue, policyData.instanceBuffer);
		render.DeleteQueue(policyData.portQueue);
	}

	renderPolicyMap.clear();
	warpData.clear();
}

TObject<IReflect>& CameraComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(prevTaskData)[Runtime];
		ReflectProperty(nextTaskData)[Runtime];
		ReflectProperty(renderFlowComponent)[Runtime];
		ReflectProperty(bridgeComponent)[Runtime];

		ReflectProperty(collectedEntityCount);
		ReflectProperty(collectedVisibleEntityCount);
		ReflectProperty(collectedTriangleCount);
		ReflectProperty(collectedDrawCallCount);
		ReflectProperty(jitterIndex)[Runtime];

		ReflectProperty(nearPlane);
		ReflectProperty(farPlane);
		ReflectProperty(fov);
		ReflectProperty(aspect);
		ReflectProperty(viewDistance);
	}

	return *this;
}

TObject<IReflect>& CameraComponentConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
	}

	return *this;
}

CameraComponentConfig::WorldGlobalData::WorldGlobalData() :
	viewProjectionMatrix(MatrixFloat4x4::Identity()), projectionMatrix(MatrixFloat4x4::Identity()), lastViewProjectionMatrix(MatrixFloat4x4::Identity()),
	viewMatrix(MatrixFloat4x4::Identity()), inverseViewMatrix(MatrixFloat4x4::Identity()), jitterMatrix(MatrixFloat4x4::Identity()),
	viewPosition(0, 0, 0), jitterOffset(0, 0) {}

TObject<IReflect>& CameraComponentConfig::WorldGlobalData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(viewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];
		ReflectProperty(lastViewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_LAST_VIEWPROJECTION)];
		ReflectProperty(viewMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(viewPosition);
		ReflectProperty(time);
		ReflectProperty(tanHalfFov);
	}

	return *this;
}

