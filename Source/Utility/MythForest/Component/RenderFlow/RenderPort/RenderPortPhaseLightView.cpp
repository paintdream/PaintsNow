#include "RenderPortPhaseLightView.h"
#include "../../Light/LightComponent.h"

using namespace PaintsNow;

RenderPortPhaseLightView::RenderPortPhaseLightView() {}

TObject<IReflect>& RenderPortPhaseLightView::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderPortPhaseLightView::Initialize(Engine& engine, IRender::Queue* mainQueue) {
}

void RenderPortPhaseLightView::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {
}

bool RenderPortPhaseLightView::OnFrameEncodeBegin(Engine& engine) {
	return true;
}

void RenderPortPhaseLightView::OnFrameEncodeEnd(Engine& engine) {
	node->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	for (size_t i = 0; i < links.size(); i++) {
		RenderPort* renderPort = static_cast<RenderPort*>(links[i].port);
		renderPort->GetNode()->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}