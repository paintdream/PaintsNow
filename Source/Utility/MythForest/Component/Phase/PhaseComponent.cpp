#include "PhaseComponent.h"
#include "../Light/LightComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <cmath>
#include <ctime>
#include <utility>

const float PI = 3.1415926f;
static inline float RandFloat() {
	return (float)rand() / (float)RAND_MAX;
}

using namespace PaintsNow;

PhaseComponent::LightConfig::TaskData::TaskData(uint32_t warpCount) : pendingCount(0), pendingResourceCount(0) {
	warpData.resize(warpCount);
}

PhaseComponent::Phase::Phase() : shadowIndex(0), drawCallResource(nullptr) {}

PhaseComponentConfig::WorldGlobalData::WorldGlobalData() : 
	viewProjectionMatrix(MatrixFloat4x4::Identity()), viewMatrix(MatrixFloat4x4::Identity()), lightReprojectionMatrix(MatrixFloat4x4::Identity()),
	lightColor(1, 1, 1, 1), lightPosition(0, 0, 0, 0), invScreenSize(1.0f, 1.0f) {}

TObject<IReflect>& PhaseComponentConfig::WorldGlobalData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(viewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];
		ReflectProperty(viewMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(lightReprojectionMatrix)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(lightColor)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(lightPosition)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(noiseTexture);
		ReflectProperty(lightDepthTexture);
	}

	return *this;
}

TObject<IReflect>& PhaseComponentConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}

PhaseComponentConfig::TaskData::TaskData() : status(STATUS_IDLE), pendingCount(0), pendingResourceCount(0), renderQueue(nullptr), renderTarget(nullptr), pipeline(nullptr)
#if !defined(_MSC_VER) || _MSC_VER > 1200
	, mergedInstancedGroupMap(&globalBytesCache)
#endif
{}

PhaseComponentConfig::TaskData::~TaskData() {
	assert(warpData.empty());
}

PhaseComponentConfig::InstanceGroup::InstanceGroup() : drawCallResource(nullptr), instanceCount(0) {}

void PhaseComponentConfig::InstanceGroup::Cleanup() {
	for (size_t i = 0; i < instancedData.size(); i++) {
		instancedData[i].Clear();
	}

	drawCallResource = nullptr;
	instanceCount = 0;
}

PhaseComponentConfig::MergedInstanceGroup::MergedInstanceGroup() : groupPrototype(nullptr), mergedInstanceCount(0) {}

void PhaseComponentConfig::TaskData::Cleanup(IRender& render) {
	OPTICK_EVENT();
	for (size_t i = 0; i < warpData.size(); i++) {
		WarpData& data = warpData[i];
		data.bytesCache.Reset();

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

	worldGlobalBufferMap.clear();
	globalBytesCache.Reset();

#if defined(_MSC_VER) && _MSC_VER <= 1200
	mergedInstancedGroupMap.clear();
#else
	std::destruct(&mergedInstancedGroupMap);
	new (&mergedInstancedGroupMap) TaskData::MergedInstanceGroupMap(&globalBytesCache);
#endif
}

void PhaseComponentConfig::TaskData::Destroy(IRender& render) {
	OPTICK_EVENT();
	Cleanup(render);
	warpData.clear();

	render.DeleteResource(renderQueue, renderTarget);
	render.DeleteQueue(renderQueue);
}

PhaseComponent::PhaseComponent(const TShared<RenderFlowComponent>& renderFlow, const String& portName) : hostEntity(nullptr), maxTracePerTick(8), renderQueue(nullptr), stateSetupResource(nullptr), statePostResource(nullptr), stateShadowResource(nullptr), range(32, 32, 32), resolution(512, 512), lightCollector(this), renderFlowComponent(std::move(renderFlow)), lightPhaseViewPortName(portName), rootEntity(nullptr) {}

PhaseComponent::~PhaseComponent() {}

void PhaseComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	if (rootEntity != entity) { // Set Host?
		BaseClass::Initialize(engine, entity);
		assert(hostEntity == nullptr);
		assert(renderQueue == nullptr);
		hostEntity = entity;

		SnowyStream& snowyStream = engine.snowyStream;
		const String path = "[Runtime]/MeshResource/StandardQuad";
		meshResource = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);

		tracePipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashTracePass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashTracePass> >());
		setupPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashSetupPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashSetupPass> >());
		shadowPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

		IRender& render = engine.interfaces.render;
		IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
		renderQueue = render.CreateQueue(device);

		do {
			statePostResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
			IRender::Resource::RenderStateDescription& state = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(renderQueue, statePostResource, 0));
			state.cull = 1;
			state.fill = 1;
			state.blend = 0;
			state.colorWrite = 1;
			state.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
			state.depthWrite = 0;
			state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
			state.stencilWrite = 0;
			render.UnmapResource(renderQueue, statePostResource, IRender::MAP_DATA_EXCHANGE);
		} while (false);

		do {
			stateSetupResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
			IRender::Resource::RenderStateDescription& state = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(renderQueue, stateSetupResource, 0));
			state.cull = 1;
			state.fill = 1;
			state.blend = 0;
			state.colorWrite = 1;
			state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
			state.depthWrite = 1;
			state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
			state.stencilWrite = 0;
			render.UnmapResource(renderQueue, stateSetupResource, IRender::MAP_DATA_EXCHANGE);
		} while (false);

		do {
			stateShadowResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
			IRender::Resource::RenderStateDescription& state = *static_cast<IRender::Resource::RenderStateDescription*>(render.MapResource(renderQueue, stateShadowResource, 0));
			state.cull = 1;
			state.fill = 1;
			state.blend = 0;
			state.colorWrite = 0;
			state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
			state.depthWrite = 1;
			state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
			state.stencilWrite = 0;
			state.cullFrontFace = 0; // It's not the same with shadow map, we must not cull front face since some internal pixels may be incorrectly lighted.
			render.UnmapResource(renderQueue, stateShadowResource, IRender::MAP_DATA_EXCHANGE);
		} while (false);
	}
}

Tiny::FLAG PhaseComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

void PhaseComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	if (rootEntity == entity) {
		rootEntity = nullptr;
	} else {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(tracePipeline->GetResourceManager().GetContext());
		IRender& render = engine.interfaces.render;
		for (size_t j = 0; j < tasks.size(); j++) {
			TaskData& task = tasks[j];
			task.Destroy(render);
		}

		tasks.clear();

		for (size_t k = 0; k < phases.size(); k++) {
			Phase& phase = phases[k];
			for (size_t j = 0; j < phase.uniformBuffers.size(); j++) {
				render.DeleteResource(queue, phase.uniformBuffers[j]);
			}

			render.DeleteResource(queue, phase.drawCallResource);
		}

		phases.clear();

		render.DeleteResource(queue, stateSetupResource);
		render.DeleteResource(queue, statePostResource);
		render.DeleteResource(queue, stateShadowResource);
		render.DeleteQueue(renderQueue);

		stateSetupResource = statePostResource = stateShadowResource = nullptr;
		renderQueue = nullptr;
		hostEntity = nullptr;

		BaseClass::Uninitialize(engine, entity);
	}
}

void PhaseComponent::Resample(Engine& engine, uint32_t phaseCount) {
	// TODO: resample phases.
}

void PhaseComponent::Step(Engine& engine, uint32_t bounceCount) {
	// generate pairs of phases
	assert(phases.size() >= 2);
	for (uint32_t i = 0; i < bounceCount; i++) {
		Phase* from = &phases[rand() % phases.size()];
		Phase* to;
		do {
			to = &phases[rand() % phases.size()];
		} while (to == from);

		UpdatePointBounce bounce;
		bounce.fromPhaseIndex = verify_cast<uint32_t>(from - &phases[0]);
		bounce.toPhaseIndex = verify_cast<uint32_t>(to - &phases[0]);

		bakePointBounces.push(bounce);
	}
}

void PhaseComponent::Setup(Engine& engine, uint32_t phaseCount, uint32_t taskCount, const Float3& r, const UShort2& res) {
	OPTICK_EVENT();
	assert(tasks.empty() && phases.empty());
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	SnowyStream& snowyStream = engine.snowyStream;
	resolution = res;
	range = r;

	uint32_t warpCount = engine.GetKernel().GetWarpCount();
	tasks.resize(taskCount);
	for (size_t j = 0; j < tasks.size(); j++) {
		TaskData& task = tasks[j];
		task.warpData.resize(warpCount);
		task.renderQueue = render.CreateQueue(device);
		task.renderTarget = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
		render.SetResourceNote(task.renderTarget, "PhaseBakePass");
	}

	// prepare uniform buffers for tracing
	phases.resize(phaseCount);
	for (size_t i = 0; i < phases.size(); i++) {
		Phase& phase = phases[i];

		std::vector<Bytes> bufferData;
		phase.tracePipeline.Reset(static_cast<ShaderResourceImpl<MultiHashTracePass>*>(tracePipeline->Clone()));
		phase.tracePipeline->GetPassUpdater().Capture(phase.drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.tracePipeline->GetPassUpdater().Update(render, renderQueue, phase.drawCallDescription, phase.uniformBuffers, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.drawCallResource = render.CreateResource(device, IRender::Resource::RESOURCE_DRAWCALL);

		phase.depth = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseDepth", &phase), false, ResourceBase::RESOURCE_VIRTUAL);

		phase.depth->description.state.immutable = false;
		phase.depth->description.state.attachment = true;
		phase.depth->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
		phase.depth->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		phase.depth->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		phase.depth->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.depth->GetResourceManager().InvokeUpload(phase.depth(), renderQueue);

		phase.irradiance = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseIrradiance", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.irradiance->description.state.immutable = false;
		phase.irradiance->description.state.attachment = true;
		phase.irradiance->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
		phase.irradiance->description.state.format = IRender::Resource::TextureDescription::HALF;
		phase.irradiance->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.irradiance->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.irradiance->GetResourceManager().InvokeUpload(phase.irradiance(), renderQueue);

		phase.baseColorOcclusion = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseBaseColorOcclusion", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.baseColorOcclusion->description.state.immutable = false;
		phase.baseColorOcclusion->description.state.attachment = true;
		phase.baseColorOcclusion->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
		phase.baseColorOcclusion->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.baseColorOcclusion->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.baseColorOcclusion->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.baseColorOcclusion->GetResourceManager().InvokeUpload(phase.baseColorOcclusion(), renderQueue);

		phase.normalRoughnessMetallic = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNormalRoughness", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.normalRoughnessMetallic->description.state.immutable = false;
		phase.normalRoughnessMetallic->description.state.attachment = true;
		phase.normalRoughnessMetallic->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
		phase.normalRoughnessMetallic->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.normalRoughnessMetallic->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.normalRoughnessMetallic->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		phase.normalRoughnessMetallic->GetResourceManager().InvokeUpload(phase.normalRoughnessMetallic(), renderQueue);

		// create noise texture
		phase.noiseTexture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNoise", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.noiseTexture->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
		phase.noiseTexture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.noiseTexture->description.state.layout = IRender::Resource::TextureDescription::RGBA;

		Bytes& bytes = phase.noiseTexture->description.data;
		bytes.Resize(resolution.x() * resolution.y() * 4);
#if RAND_MAX == 0x7FFFFFFF
		uint32_t* buffer = reinterpret_cast<uint32_t*>(bytes.GetData());
		for (size_t k = 0; k < resolution.x() * resolution.y() * 4 / sizeof(uint32_t); k++) {
			buffer[k] = rand() | (rand() & 1 ? 0x80000000 : 0);
		}
#elif RAND_MAX == 0x7FFF
		uint16_t* buffer = reinterpret_cast<uint16_t*>(bytes.GetData());
		for (size_t k = 0; k < resolution.x() * resolution.y() * 4 / sizeof(uint16_t); k++) {
			buffer[k] = rand() | (rand() & 1 ? 0x8000 : 0);
		}
#else
		static_assert(false, "Unrecognized RAND_MAX");
#endif
		phase.noiseTexture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.noiseTexture->GetResourceManager().InvokeUpload(phase.noiseTexture(), renderQueue);
	}
}

void PhaseComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (entity != rootEntity) {
		if (event.eventID == Event::EVENT_FRAME) {
			// DoUpdate
			TickRender(event.engine);
		} else if (event.eventID == Event::EVENT_TICK) {
			Engine& engine = event.engine;
			ResolveTasks(engine);
			DispatchTasks(engine);
			UpdateRenderFlow(engine);
		}
	}
}

// #include "../../../MythForest/MythForest.h"

void PhaseComponent::TickRender(Engine& engine) {
	OPTICK_EVENT();
	// check pipeline state
	if (renderQueue == nullptr) return; // not inited.

	IRender& render = engine.interfaces.render;
	Kernel& kernel = engine.GetKernel();
	render.SubmitQueues(&renderQueue, 1, IRender::SUBMIT_EXECUTE_ALL);

	std::vector<IRender::Queue*> bakeQueues;
	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			uint32_t status = TaskData::STATUS_BAKED;
			if (!debugPath.empty()) {
				for (size_t k = 0; k < task.textures.size(); k++) {
					IRender::Resource::TextureDescription& target = task.textures[k]->description;
					target.data.Clear();

					IRender::Resource* texture = task.textures[k]->GetRenderResource();
					IRender::Resource::TextureDescription* p = static_cast<IRender::Resource::TextureDescription*>(render.MapResource(task.renderQueue, texture, IRender::MAP_DATA_EXCHANGE));
					if (p != nullptr) {
						target = std::move(*p);
						render.UnmapResource(task.renderQueue, texture, 0);
					}
				}

				status = TaskData::STATUS_DOWNLOADED;
			}

			// engine.mythForest.StartCaptureFrame("cap", "");
			render.FlushQueue(task.renderQueue);
			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(status, std::memory_order_release);
			// render.SubmitQueues(&task.renderQueue, 1, IRender::SUBMIT_EXECUTE_ALL);
			// engine.mythForest.EndCaptureFrame();
		} else if (task.status == TaskData::STATUS_DOWNLOADED) {
			uint32_t frameIndex = engine.snowyStream.GetRenderResourceManager()->GetFrameIndex();
			uint32_t status = TaskData::STATUS_BAKED;
			for (size_t k = 0; k < task.textures.size(); k++) {
				IRender::Resource::TextureDescription& target = task.textures[k]->description;
				if (target.data.Empty()) {
					IRender::Resource* texture = task.textures[k]->GetRenderResource();
					IRender::Resource::TextureDescription* p = static_cast<IRender::Resource::TextureDescription*>(render.MapResource(task.renderQueue, texture, IRender::MAP_DATA_EXCHANGE));
					if (p != nullptr) {
						target = std::move(*p);
						render.UnmapResource(task.renderQueue, texture, 0);
					} else {
						status = TaskData::STATUS_DOWNLOADED; // not finished!
						break;
					}
				}
			}

			// all finished!
			if (status == TaskData::STATUS_BAKED) {
				for (size_t k = 0; k < task.textures.size(); k++) {
					IRender::Resource::TextureDescription& target = task.textures[k]->description;
					assert(!target.data.Empty()); // must success
					engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &PhaseComponent::CoTaskWriteDebugTexture), std::ref(engine), (uint32_t)verify_cast<uint32_t>(frameIndex * tasks.size() + i), std::move(task.textures[k]->description.data), task.textures[k]));
				}
			}

			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.SubmitQueues(&bakeQueues[0], verify_cast<uint32_t>(bakeQueues.size()), IRender::SUBMIT_EXECUTE_ALL);
	}
}

static String ParseStageFromTexturePath(const String& path) {
	char stage[256] = "Unknown";
	const char* p = path.c_str() + 17;
	while (*p != '/') p++;

	return String(path.c_str() + 17, p);
}

void PhaseComponent::CoTaskWriteDebugTexture(Engine& engine, uint32_t index, Bytes& data, const TShared<TextureResource>& texture) {
	OPTICK_EVENT();
	if (!debugPath.empty()) {
		std::stringstream ss;
		ss << debugPath << "phase_" << index << "_" << ParseStageFromTexturePath(texture->GetLocation()) << ".png";
		uint64_t length;
		IStreamBase* stream = engine.interfaces.archive.Open(StdToUtf8(ss.str()), true, length);
		IRender::Resource::TextureDescription& description = texture->description;
		IImage& image = engine.interfaces.image;
		IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)description.state.layout;
		IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)description.state.format;
		length = data.GetSize();

		if (layout == IRender::Resource::TextureDescription::DEPTH) {
			layout = IRender::Resource::TextureDescription::R;
			format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			size_t count = description.dimension.x() * description.dimension.y();
			const float* base = (const float*)data.GetData();
			uint8_t* target = (uint8_t*)data.GetData();
			length = sizeof(uint8_t) * count;

			for (size_t i = 0; i < count; i++) {
				target[i] = (uint8_t)(Math::Clamp(base[i], 0.0f, 1.0f) * 0xff);
			}
		} else if (layout == IRender::Resource::TextureDescription::RGBA && format == IRender::Resource::TextureDescription::HALF) {
			layout = IRender::Resource::TextureDescription::RGBA;
			format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			size_t count = description.dimension.x() * description.dimension.y();
			const uint16_t* base = (const uint16_t*)data.GetData();
			uint8_t* target = (uint8_t*)data.GetData();
			length = sizeof(uint8_t) * count * 4;

			for (size_t i = 0; i < count * 4; i++) {
				target[i] = (uint8_t)(Math::Clamp(Math::HalfToFloat(base[i]), 0.0f, 1.0f) * 0xff);
			}
		}

		IImage::Image* png = image.Create(description.dimension.x(), description.dimension.y(), layout, format);
		void* buffer = image.GetBuffer(png);
		memcpy(buffer, data.GetData(), verify_cast<size_t>(length));
		image.Save(png, *stream, "png");
		image.Delete(png);
		// write png
		stream->Destroy();
	}
}

void PhaseComponent::ResolveTasks(Engine& engine) {
	OPTICK_EVENT();
	// resolve finished tasks
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& taskData = tasks[k];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(taskData.status);
		if (taskData.status == TaskData::STATUS_BAKED) {
			// TODO: finish baked
			finalStatus.store(TaskData::STATUS_IDLE);
		} else if (taskData.status == TaskData::STATUS_ASSEMBLING) {
			if (taskData.pendingCount == 0) {
				// Commit draw calls.
				IRender::Queue* queue = taskData.renderQueue;
				IRender& render = engine.interfaces.render;
				IRender::Device* device = render.GetQueueDevice(queue);
				IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);

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
				}

				Bytes instanceData;
				uint32_t instanceOffset = 0;
				std::vector<IRender::Resource*> drawCallResources;
				std::vector<IRender::Resource*> globalBufferResources;
				// Generate all render resources.
				for (TaskData::MergedInstanceGroupMap::iterator it = taskData.mergedInstancedGroupMap.begin(); it != taskData.mergedInstancedGroupMap.end(); ++it) {
					InstanceGroup& group = *(*it).second.groupPrototype;
					assert(group.instanceCount != 0);
					assert(group.drawCallDescription.GetShader() != nullptr);

					ShaderResource* shaderResource = group.shaderResource();

					// Generate buffer for global data
					PassBase::Updater& updater = shaderResource->GetPassUpdater();
					std::vector<KeyValue<ShaderResource*, TaskData::GlobalBufferItem> >::iterator ig = BinaryFind(taskData.worldGlobalBufferMap.begin(), taskData.worldGlobalBufferMap.end(), shaderResource);

					if (ig == taskData.worldGlobalBufferMap.end()) {
						ig = BinaryInsert(taskData.worldGlobalBufferMap, shaderResource);
						taskData.worldGlobalData.Export(ig->second.globalUpdater, updater);

						std::vector<Bytes, TCacheAllocator<Bytes> > s(&taskData.globalBytesCache);
						std::vector<IRender::Resource::DrawCallDescription::BufferRange, TCacheAllocator<IRender::Resource::DrawCallDescription::BufferRange> > bufferResources(&taskData.globalBytesCache);
						std::vector<IRender::Resource*, TCacheAllocator<IRender::Resource*> > textureResources(&taskData.globalBytesCache);
						ig->second.globalUpdater.Snapshot(s, bufferResources, textureResources, taskData.worldGlobalData, &taskData.globalBytesCache);
						ig->second.buffers.resize(updater.GetBufferCount());

						for (size_t i = 0; i < s.size(); i++) {
							Bytes& data = s[i];
							if (!data.Empty()) {
								IRender::Resource* res = render.CreateResource(device, IRender::Resource::RESOURCE_BUFFER);
								IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, res, 0));
								desc.state.usage = IRender::Resource::BufferDescription::UNIFORM;
								desc.state.component = 4;
								desc.state.dynamic = 1;
								desc.state.format = IRender::Resource::BufferDescription::FLOAT;
								desc.data = std::move(data);
								render.UnmapResource(queue, res, IRender::MAP_DATA_EXCHANGE);
								ig->second.buffers[i].buffer = res;
								globalBufferResources.emplace_back(res);
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

					group.drawCallDescription.instanceCount = verify_cast<uint16_t>(mergedInstanceCount);

					// Generate DrawCall
					assert(PassBase::ValidateDrawCall(group.drawCallDescription));

					IRender::Resource*& drawCall = group.drawCallResource;
					drawCall = render.CreateResource(device, IRender::Resource::RESOURCE_QUICK_DRAWCALL, queue);
					IRender::Resource::QuickDrawCallDescription& deviceDrawCall = *static_cast<IRender::Resource::QuickDrawCallDescription*>(render.MapResource(queue, drawCall, 0));
					deviceDrawCall = group.drawCallDescription;
					render.UnmapResource(queue, drawCall, IRender::MAP_DATA_EXCHANGE);
					drawCallResources.emplace_back(drawCall);
				}

				IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
				desc.data.Resize(instanceOffset);
				desc.data.Import(0, instanceData);
				assert(instanceOffset == desc.data.GetSize());
				desc.state.format = IRender::Resource::BufferDescription::FLOAT;
				desc.state.usage = IRender::Resource::BufferDescription::INSTANCED;
				desc.state.dynamic = 1;
				desc.state.component = 0; // will be overridden by drawcall.
				render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);

				for (size_t i = 0; i < drawCallResources.size(); i++) {
					IRender::Resource* drawCall = drawCallResources[i];
					render.ExecuteResource(queue, drawCall);
					// cleanup at current frame
					render.DeleteResource(queue, drawCall);
				}

				for (size_t n = 0; n < globalBufferResources.size(); n++) {
					render.DeleteResource(queue, globalBufferResources[n]);
				}

				render.DeleteResource(queue, buffer);
				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void PhaseComponent::TaskAssembleTaskBounce(Engine& engine, TaskData& task, const UpdatePointBounce& bakePoint) {
	OPTICK_EVENT();
	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);

	IRender::Resource::RenderTargetDescription::Storage storage;
	const Phase& fromPhase = phases[bakePoint.fromPhaseIndex];
	Phase& toPhase = phases[bakePoint.toPhaseIndex];
	storage.resource = toPhase.irradiance->GetRenderResource();
	storage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	storage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	// task.texture = toPhase.irradiance;
	task.textures.clear();
	task.textures.emplace_back(toPhase.irradiance);

	// TODO: fill params
	MultiHashTraceFS& fs = static_cast<MultiHashTracePass&>(toPhase.tracePipeline->GetPass()).shaderMultiHashTrace;

	// changing state
	IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(task.renderQueue, task.renderTarget, 0));
	desc.colorStorages.clear();
	desc.colorStorages.emplace_back(storage);
	desc.depthStorage.loadOp = desc.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD; // TODO:
	desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
	desc.dimension = UShort3(resolution.x(), resolution.y(), 0);
	render.UnmapResource(task.renderQueue, task.renderTarget, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(task.renderQueue, statePostResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	// encode draw call
	std::vector<IRender::Resource*> placeholders;
	PassBase::Updater& updater = toPhase.tracePipeline->GetPassUpdater();
	std::vector<Bytes> bufferData;
	updater.Capture(toPhase.drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
	updater.Update(render, task.renderQueue, toPhase.drawCallDescription, placeholders, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
	assert(placeholders.empty());

	IRender::Resource::DrawCallDescription& dc = *static_cast<IRender::Resource::DrawCallDescription*>(render.MapResource(task.renderQueue, toPhase.drawCallResource, 0));
	dc = std::move(toPhase.drawCallDescription);
	render.UnmapResource(task.renderQueue, toPhase.drawCallResource, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(task.renderQueue, toPhase.drawCallResource);
}

void PhaseComponent::CoTaskAssembleTaskShadow(Engine& engine, TaskData& task, const UpdatePointShadow& bakePoint) {
	OPTICK_EVENT();

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	const Shadow& shadow = shadows[bakePoint.shadowIndex];
	IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(task.renderQueue, task.renderTarget, 0));
	desc.depthStorage.resource = shadow.shadow->GetRenderResource();
	desc.depthStorage.loadOp = desc.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	render.UnmapResource(task.renderQueue, task.renderTarget, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(task.renderQueue, stateShadowResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.pipeline = shadowPipeline();
	task.textures.clear();
	task.textures.emplace_back(shadow.shadow);
	// task.texture = nullptr;
	Collect(engine, task, shadow.viewMatrix, shadow.viewMatrix * shadow.projectionMatrix);
}

void PhaseComponent::CompleteCollect(Engine& engine, TaskData& taskData) {}

void PhaseComponent::Collect(Engine& engine, TaskData& taskData, const MatrixFloat4x4& viewMatrix, const MatrixFloat4x4& worldMatrix) {
	OPTICK_EVENT();
	PerspectiveCamera camera;
	CaptureData captureData;
	camera.UpdateCaptureData(captureData, Math::QuickInverse(viewMatrix));
	assert(rootEntity->GetWarpIndex() == GetWarpIndex());
	WorldInstanceData instanceData;
	instanceData.worldMatrix = worldMatrix;
	instanceData.instancedColor.x() = 1.0f / resolution.x();
	instanceData.instancedColor.y() = 1.0f / resolution.y();
	taskData.Cleanup(engine.interfaces.render);

	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(taskData.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, taskData, instanceData, captureData, rootEntity);
}

void PhaseComponent::CoTaskAssembleTaskSetup(Engine& engine, TaskData& task, const UpdatePointSetup& bakePoint) {
	OPTICK_EVENT();

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	Phase& phase = phases[bakePoint.phaseIndex];

	TextureResource* rt[] = {
		phase.irradiance(),
		phase.baseColorOcclusion(),
		phase.normalRoughnessMetallic()
	};

	IRender::Resource::RenderTargetDescription& desc = *static_cast<IRender::Resource::RenderTargetDescription*>(render.MapResource(task.renderQueue, task.renderTarget, 0));
	desc.colorStorages.clear();
	for (size_t k = 0; k < sizeof(rt) / sizeof(rt[0]); k++) {
		IRender::Resource::RenderTargetDescription::Storage storage;
		storage.resource = rt[k]->GetRenderResource();
		storage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		storage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
		desc.colorStorages.emplace_back(storage);
	}

	desc.depthStorage.resource = phase.depth->GetRenderResource();
	desc.depthStorage.loadOp = desc.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	render.UnmapResource(task.renderQueue, task.renderTarget, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(task.renderQueue, stateSetupResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.textures.clear();
	task.textures.emplace_back(phase.irradiance);
	task.textures.emplace_back(phase.baseColorOcclusion);
	task.textures.emplace_back(phase.normalRoughnessMetallic);

	task.pipeline = setupPipeline();
	task.camera = phase.camera;
	task.worldGlobalData.noiseTexture.resource = phase.noiseTexture->GetRenderResource();
	task.worldGlobalData.viewMatrix = phase.viewMatrix;
	task.worldGlobalData.viewProjectionMatrix = phase.viewMatrix * phase.projectionMatrix;

	// shadow
	phase.shadowIndex = bakePoint.shadowIndex;
	Shadow& shadow = shadows[phase.shadowIndex];
	const UShort3& dim = shadow.shadow->description.dimension;
	task.worldGlobalData.lightReprojectionMatrix = Math::InversePerspective(phase.projectionMatrix) * Math::QuickInverse(phase.viewMatrix) * shadow.viewMatrix * shadow.projectionMatrix;
	task.worldGlobalData.lightColor = shadow.lightElement.colorAttenuation;
	task.worldGlobalData.lightPosition = shadow.lightElement.position;
	task.worldGlobalData.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
	task.worldGlobalData.lightDepthTexture.resource = shadow.shadow->GetRenderResource();

	Collect(engine, task, phase.viewMatrix, MatrixFloat4x4::Identity());
}

void PhaseComponent::DispatchTasks(Engine& engine) {
	size_t n = 0;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[n];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_START) {
			if (!bakePointShadows.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointShadow& shadow = bakePointShadows.top();
				threadPool.Dispatch(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskShadow), std::ref(engine), std::ref(task), shadow));
				bakePointShadows.pop();
			} else if (!bakePointSetups.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointSetup& setup = bakePointSetups.top();
				threadPool.Dispatch(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskSetup), std::ref(engine), std::ref(task), setup));
				bakePointSetups.pop();
			} else if (!bakePointBounces.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointBounce& bounce = bakePointBounces.top();
				TaskAssembleTaskBounce(engine, task, bounce);
				bakePointBounces.pop();
			}
		}
	}
}

void PhaseComponent::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = taskData.renderQueue;
	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	TaskData::WarpData& warpData = taskData.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];

	IDrawCallProvider::InputRenderData inputRenderData(0.0f, taskData.pipeline);
	IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
	std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
	renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_AGILE_RENDERING);
	assert(drawCalls.size() < sizeof(RenderableComponent) - 1);

	for (size_t k = 0; k < drawCalls.size(); k++) {
		// PassBase& Pass = provider->GetPass(k);
		IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
		const IRender::Resource::QuickDrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;

		// Generate key
		InstanceKey key = (InstanceKey)drawCall.hashValue;
		InstanceGroup& group = warpData.instanceGroups[key];
		if (group.instanceCount == 0) {
			group.drawCallDescription = std::move(drawCallTemplate);
			group.shaderResource = drawCall.shaderResource;
			PassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();
			instanceData.Export(group.instanceUpdater, updater);
		}

		std::vector<IRender::Resource*> textureResources;
		std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
		group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
		group.instanceCount++;
	}
}

void PhaseComponent::CompleteUpdateLights(Engine& engine, std::vector<LightElement>& elements) {
	OPTICK_EVENT();
	MatrixFloat4x4 projectionMatrix = Math::MatrixOrtho(range);
	bakePointShadows = std::stack<UpdatePointShadow>();
	shadows.resize(elements.size());

	// generate shadow tasks
	for (size_t i = 0; i < elements.size(); i++) {
		Shadow& shadow = shadows[i];
		const LightElement& lightElement = elements[i];
		if (!shadow.shadow) {
			shadow.shadow = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseShadow", &shadow), false, ResourceBase::RESOURCE_VIRTUAL);
			shadow.shadow->description.state.immutable = false;
			shadow.shadow->description.state.attachment = true;
			shadow.shadow->description.dimension = UShort3(resolution.x(), resolution.y(), 0);
			shadow.shadow->description.state.format = IRender::Resource::TextureDescription::FLOAT;
			shadow.shadow->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
			shadow.shadow->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
			shadow.shadow->GetResourceManager().InvokeUpload(shadow.shadow(), renderQueue);
		}

		assert(hostEntity != nullptr);
		TransformComponent* transformComponent = hostEntity->GetUniqueComponent(UniqueType<TransformComponent>());
		Float3 center;
		if (transformComponent != nullptr) {
			center = transformComponent->GetQuickTranslation();
		}

		shadow.lightElement = lightElement;
		Float3 view = (Float3)lightElement.position;
		Float3 dir = -view;
		if (Math::SquareLength(dir) < 1e-6) {
			dir = Float3(0, 0, -1);
		}

		Float3 up(RandFloat(), RandFloat(), RandFloat());

		shadow.viewMatrix = Math::MatrixLookAt(view, dir, up);
		shadow.projectionMatrix = projectionMatrix;

		UpdatePointShadow bakePointShadow;
		bakePointShadow.shadowIndex = verify_cast<uint32_t>(i);
		bakePointShadows.push(bakePointShadow);
	}

	// generate setup tasks
	bakePointSetups = std::stack<UpdatePointSetup>();
	if (!elements.empty()) {
		for (size_t j = 0; j < phases.size(); j++) {
			UpdatePointSetup bakePoint;
			bakePoint.phaseIndex = verify_cast<uint32_t>(j);
			bakePoint.shadowIndex = verify_cast<uint32_t>(rand() % elements.size());
			bakePointSetups.push(bakePoint);
		}
	}

	// stop all bounces
	// bakePointBounces = std::stack<UpdatePointBounce>();
}

PhaseComponent::LightCollector::LightCollector(PhaseComponent* component) : phaseComponent(component) {}

void PhaseComponent::LightCollector::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	size_t size = entity->GetComponentCount();
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	if (transformComponent != nullptr) {
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if (!captureData(localBoundingBox))
			return;
	}

	WorldInstanceData instanceData;
	instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;
	instanceData.boundingBox = inst.boundingBox;

	for (size_t i = 0; i < size; i++) {
		Component* component = entity->GetComponent(i);
		if (component != nullptr) {
			uint32_t flag = component->GetEntityFlagMask();
			std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
			if (flag & Entity::ENTITY_HAS_SPACE) {
				counter.fetch_add(1, std::memory_order_release);
				CollectComponentsFromSpace(engine, task, instanceData, captureData, static_cast<SpaceComponent*>(component));
			} else if (flag & Entity::ENTITY_HAS_RENDERCONTROL) {
				LightComponent* lightComponent = component->QueryInterface(UniqueType<LightComponent>());
				if (lightComponent != nullptr) {
					LightElement element;
					const MatrixFloat4x4& worldMatrix = instanceData.worldMatrix;
					if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
						element.position = Float4(-worldMatrix(2, 0), -worldMatrix(2, 1), -worldMatrix(2, 2), 0);
					} else {
						// Only directional light by now
						element.position = Float4(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2), 1);
					}

					const Float3& color = lightComponent->GetColor();
					element.colorAttenuation = Float4(color.x(), color.y(), color.z(), lightComponent->GetAttenuation());
					task.warpData[engine.GetKernel().GetCurrentWarpIndex()].lightElements.emplace_back(element);
				}
			}
		}
	}
}

void PhaseComponent::LightCollector::CompleteCollect(Engine& engine, TaskData& taskData) {
	OPTICK_EVENT();
	std::vector<LightElement> lightElements;
	for (size_t i = 0; i < taskData.warpData.size(); i++) {
		LightConfig::WarpData& warpData = taskData.warpData[i];
		std::copy(warpData.lightElements.begin(), warpData.lightElements.end(), std::back_inserter(lightElements));
	}

	engine.GetKernel().QueueRoutine(phaseComponent, CreateTaskContextFree(Wrap(phaseComponent, &PhaseComponent::CompleteUpdateLights), std::ref(engine), lightElements));
	delete& taskData;
	phaseComponent->ReleaseObject();
}

void PhaseComponent::LightCollector::InvokeCollect(Engine& engine, Entity* entity) {
	LightConfig::TaskData* taskData = new LightConfig::TaskData(engine.GetKernel().GetWarpCount());
	phaseComponent->ReferenceObject();
	LightConfig::WorldInstanceData worldInstance;
	worldInstance.worldMatrix = MatrixFloat4x4::Identity();
	worldInstance.boundingBox = Float3Pair(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	LightConfig::CaptureData captureData;

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, *taskData, worldInstance, captureData, entity);
}

void PhaseComponent::Update(Engine& engine, const Float3& center) {
	OPTICK_EVENT();
	// generate bake points
	srand(0);
	bakePointSetups = std::stack<UpdatePointSetup>();

	PerspectiveCamera camera;
	camera.nearPlane = 0.1f;
	camera.farPlane = 100.0f;
	camera.aspect = 1.0f;
	camera.fov = PI / 2;

	MatrixFloat4x4 projectionMatrix = Math::MatrixPerspective(camera.fov, camera.aspect, camera.nearPlane, camera.farPlane);

	// Adjust phases?
	for (size_t i = 0; i < phases.size(); i++) {
		float theta = RandFloat() * 2 * PI;
		float phi = RandFloat() * PI;
		Float3 view = range * Float3(cos(theta), sin(theta), cos(phi));
		Float3 dir = -view;
		Float3 up(RandFloat(), RandFloat(), RandFloat());

		phases[i].camera = camera;
		phases[i].projectionMatrix = projectionMatrix;
		phases[i].viewMatrix = Math::MatrixLookAt(view + center, dir, up);
	}

	lightCollector.InvokeCollect(engine, rootEntity);
}

void PhaseComponent::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_relaxed);
	const Float3Pair& box = entity->GetKey();
	if ((rootFlag & Tiny::TINY_ACTIVATED) && captureData(box)) {
		TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
		WorldInstanceData instanceData;
		instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;
		instanceData.instancedColor = inst.instancedColor;

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
					if (!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
						// generate instance color
						if (renderableComponent->GetVisible()) {
							instanceData.instancedColor.z() = RandFloat();
							instanceData.instancedColor.w() = RandFloat();
							CollectRenderableComponent(engine, task, renderableComponent, instanceData);
						}
					}
				}
			} else if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
				std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
				counter.fetch_add(1, std::memory_order_release);
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
}

const std::vector<PhaseComponent::Phase>& PhaseComponent::GetPhases() const {
	return phases;
}

void PhaseComponent::UpdateRenderFlow(Engine& engine) {
	OPTICK_EVENT();
	RenderPort* renderPort = renderFlowComponent->BeginPort(lightPhaseViewPortName);
	if (renderPort != nullptr) {
		RenderPortPhaseLightView* phaseLightView = renderPort->QueryInterface(UniqueType<RenderPortPhaseLightView>());
		if (phaseLightView != nullptr) {
			// Update phases
			for (size_t i = 0; i < phases.size(); i++) {
				phaseLightView->phases[i] = phases[i];
			}
		}

		renderFlowComponent->EndPort(renderPort);
	}
}

void PhaseComponent::BindRootEntity(Engine& engine, Entity* entity) {
	if (rootEntity != nullptr) {
		// free last listener
		rootEntity->RemoveComponent(engine, this);
	}

	rootEntity = entity;

	if (entity != nullptr) {
		entity->AddComponent(engine, this); // weak component
	}
}

void PhaseComponent::SetDebugMode(const String& path) {
	debugPath = path;
}

