#include "RenderPortLightSource.h"
#include "../../Light/LightComponent.h"

using namespace PaintsNow;

RenderPortLightSource::RenderPortLightSource() : stencilMask(0), stencilShadow(0), cubeStrength(1.0f) {}

TObject<IReflect>& RenderPortLightSource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderPortLightSource::Initialize(Engine& engine, IRender::Queue* mainQueue) {
}

void RenderPortLightSource::Uninitialize(Engine& engine, IRender::Queue* mainQueue) {
}

bool RenderPortLightSource::OnFrameEncodeBegin(Engine& engine) {
	lightElements.clear();
	return true;
}

void RenderPortLightSource::OnFrameEncodeEnd(Engine& engine) {
	node->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	for (size_t i = 0; i < links.size(); i++) {
		RenderPort* renderPort = static_cast<RenderPort*>(links[i].port);
		renderPort->GetNode()->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}