#include "TextureArrayResource.h"
#include "../Manager/RenderResourceManager.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"
using namespace PaintsNow;

TextureArrayResource::TextureArrayResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), instance(nullptr), deviceMemoryUsage(0) {}

void TextureArrayResource::Refresh(IRender& render, void* deviceContext) {}
void TextureArrayResource::Download(IRender& render, void* deviceContext) {}
void TextureArrayResource::Upload(IRender& render, void* deviceContext) {
	if (Flag().load(std::memory_order_acquire) & RESOURCE_UPLOADED)
		return;
	OPTICK_EVENT();
	assert(!slices.empty());

	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	if (Flag().fetch_and(~TINY_MODIFIED) & TINY_MODIFIED) {
		ThreadPool& threadPool = resourceManager.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
			IRender::Resource::TextureDescription& description = *static_cast<IRender::Resource::TextureDescription*>(render.MapResource(queue, instance, 0));
			description.state = slices[0]->description.state;
			description.dimension = slices[0]->description.dimension;
			assert(description.dimension.z() == 0);
			description.dimension.z() = verify_cast<uint16_t>(slices.size());

			// concat data, slices already mapped before uploading
			size_t sliceSize = slices[0]->description.data.GetSize();
			deviceMemoryUsage = description.data.GetSize() * slices.size();

			description.data.Resize(deviceMemoryUsage);
			for (size_t i = 0; i < slices.size(); i++) {
				assert(slices[i]->description.data.GetSize() == sliceSize);
				assert(slices[i]->description.state == slices[0]->description.state);
				assert(slices[i]->description.dimension == slices[0]->description.dimension);
				memcpy(&description.data[i * sliceSize], slices[i]->description.data.GetData(), sliceSize);
			}

#ifdef _DEBUG
			render.SetResourceNote(instance, GetLocation());
#endif
			render.UnmapResource(queue, instance, IRender::MAP_DATA_EXCHANGE);

			SpinUnLock(critical);
			RenderResourceBase::Upload(render, deviceContext);
		}
	}
}

void TextureArrayResource::Attach(IRender& render, void* deviceContext) {
	BaseClass::Attach(render, deviceContext);
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);
	assert(instance == nullptr);
	instance = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_TEXTURE);
}

void TextureArrayResource::Detach(IRender& render, void* deviceContext) {
	if (instance != nullptr) {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		render.DeleteResource(queue, instance);
		instance = nullptr;
	}

	BaseClass::Detach(render, deviceContext);
}

TObject<IReflect>& TextureArrayResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(slices)[MetaResourceInternalPersist(resourceManager)];
	}

	return *this;
}

bool TextureArrayResource::Map() {
	if (!ResourceBase::Map()) return false;

	bool allDone = false;

	for (size_t i = 0; i < slices.size(); i++) {
		const TShared<TextureResource>& texture = slices[i];
		allDone = (texture->Map() && (i == 0 || (slices[0]->description.state == texture->description.state
				&& slices[0]->description.dimension == texture->description.dimension))) && allDone;
	}

	if (allDone) {
		Flag().fetch_or(TEXTUREARRAYRESOUCE_SLICE_MAPPED, std::memory_order_release);
	} else {
		Flag().fetch_or(RESOURCE_INVALID | TEXTUREARRAYRESOUCE_SLICE_MAPPED, std::memory_order_release);
	}

	return allDone;
}

bool TextureArrayResource::UnMap() {
	if (Flag().load(std::memory_order_acquire) & TEXTUREARRAYRESOUCE_SLICE_MAPPED) {
		for (size_t i = 0; i < slices.size(); i++) {
			slices[i]->UnMap();
		}
	}

	return BaseClass::UnMap();
}

