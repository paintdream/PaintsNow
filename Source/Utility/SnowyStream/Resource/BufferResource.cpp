#include "BufferResource.h"
#include "../ResourceManager.h"
#include "../Manager/RenderResourceManager.h"
#include "../../../General/Interface/IShader.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

BufferResource::BufferResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

BufferResource::~BufferResource() {}

void BufferResource::Attach(IRender& render, void* deviceContext) {
	// delayed to upload
	BaseClass::Attach(render, deviceContext);
}

TObject<IReflect>& BufferResource::operator () (IReflect& reflect) {
	if (reflect.IsReflectProperty()) {
		// serializable
		IRender::Resource::BufferDescription::State& state = description.state;
		Bytes& data = description.data;

		ReflectProperty(state);
		ReflectProperty(data);
	}

	return *this;
}

void BufferResource::Upload(IRender& render, void* deviceContext) {
	if (Flag().load(std::memory_order_acquire) & RESOURCE_UPLOADED)
		return;

	// Update buffers ...
	if (Flag().fetch_and(~TINY_MODIFIED) & TINY_MODIFIED) {
		OPTICK_EVENT();
		ThreadPool& threadPool = resourceManager.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
			IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
			assert(queue != nullptr);
			if (buffer == nullptr) {
				buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
			}

			IRender::Resource::BufferDescription& targetDescription = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
			targetDescription = description;
#ifdef _DEBUG
			render.SetResourceNote(buffer, GetLocation());
#endif
			render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);

			SpinUnLock(critical);
			RenderResourceBase::Upload(render, deviceContext);
		}
	}
}

bool BufferResource::UnMap() {
	OPTICK_EVENT();

	if (BaseClass::UnMap()) {
		ThreadPool& threadPool = resourceManager.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
			if (mapCount.load(std::memory_order_acquire) == 0) {
				description.data.Clear();
			}

			SpinUnLock(critical);
		}

		return true;
	} else {
		return false;
	}
}

void BufferResource::Detach(IRender& render, void* deviceContext) {
	OPTICK_EVENT();
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);

	RenderResourceBase::Detach(render, deviceContext);
}

void BufferResource::Download(IRender& render, void* deviceContext) {
	// download from device is not supported now
	assert(false);
}

void BufferResource::Refresh(IRender& render, void* deviceContext) {}

size_t BufferResource::ReportDeviceMemoryUsage() const {
	return description.state.length;
}
