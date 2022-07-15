#include "RasterizeComponentModule.h"

using namespace PaintsNow;

CREATE_MODULE(RasterizeComponentModule);
RasterizeComponentModule::RasterizeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& RasterizeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestRenderMesh)[ScriptMethodLocked = "RenderMesh"];
	}

	return *this;
}

TShared<RasterizeComponent> RasterizeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<TextureResource> texture) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(texture);

	TShared<RasterizeComponent> rasterizeComponent = TShared<RasterizeComponent>::From(allocator->New(texture.Get()));
	rasterizeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return rasterizeComponent;
}

void RasterizeComponentModule::RequestRenderMesh(IScript::Request& request, IScript::Delegate<RasterizeComponent> rasterizeComponent, IScript::Delegate<MeshResource> meshResource, const MatrixFloat4x4& transform) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(meshResource);

	rasterizeComponent->RenderMesh(meshResource.Get(), transform);
}

