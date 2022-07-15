#include "RenderResourceBase.h"
#include "../Manager/RenderResourceManager.h"

using namespace PaintsNow;

RenderResourceBase::RenderResourceBase(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	runtimeVersion.store(0, std::memory_order_relaxed);
}

RenderResourceBase::~RenderResourceBase() {}

RenderResourceManager& RenderResourceBase::GetRenderResourceManager() {
	return static_cast<RenderResourceManager&>(resourceManager);
}

bool RenderResourceBase::Complete(size_t version) {
	size_t currentVersion = runtimeVersion.load(std::memory_order_acquire);
	if (currentVersion > version) return false;

	Flag().fetch_or(RESOURCE_UPLOADED, std::memory_order_relaxed);
	return runtimeVersion.compare_exchange_strong(currentVersion, version, std::memory_order_acquire);
}

void RenderResourceBase::Refresh(IRender& render, void* deviceContext) {

}

void RenderResourceBase::Attach(IRender& render, void* deviceContext) {
}

void RenderResourceBase::Detach(IRender& render, void* deviceContext) {
}

struct RenderResourceBase::ResourceUploadCallback {
	TShared<RenderResourceBase> resource;

	void operator ()(IRender& render, IRender::Queue* queue, IRender::Resource* event, bool success) {
		resource->OnResourceUploaded(render, queue, event, success);
	}
};

void RenderResourceBase::OnResourceUploaded(IRender& render, IRender::Queue* queue, IRender::Resource* event, bool success) {
	if (success) {
		GetRenderResourceManager().NotifyCompletion(this);
	}

	render.DeleteResource(queue, event);
}

void RenderResourceBase::Upload(IRender& render, void* deviceContext) {
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	IRender::Resource* eventResourcePrepared = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_EVENT);
	IRender::Resource::EventDescription& desc = *static_cast<IRender::Resource::EventDescription*>(render.MapResource(queue, eventResourcePrepared, 0));
	ResourceUploadCallback callback;
	callback.resource = this;
	desc.eventCallback = WrapClosure(std::move(callback), &ResourceUploadCallback::operator ());
	render.UnmapResource(queue, eventResourcePrepared, IRender::MAP_DATA_EXCHANGE);
	render.ExecuteResource(queue, eventResourcePrepared);
}

TObject<IReflect>& RenderResourceBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
