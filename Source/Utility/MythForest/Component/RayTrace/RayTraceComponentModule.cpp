#include "RayTraceComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

CREATE_MODULE(RayTraceComponentModule);
RayTraceComponentModule::RayTraceComponentModule(Engine& engine) : BaseClass(engine) {}
RayTraceComponentModule::~RayTraceComponentModule() {}

TObject<IReflect>& RayTraceComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
		ReflectMethod(RequestGetTotalPixelCount)[ScriptMethodLocked = "GetTotalPixelCount"];
		ReflectMethod(RequestGetCompletedPixelCount)[ScriptMethodLocked = "GetCompletedPixelCount"];
		ReflectMethod(RequestConfigure)[ScriptMethodLocked = "Configure"];
		ReflectMethod(RequestSetOutputPath)[ScriptMethodLocked = "SetOutputPath"];
		ReflectMethod(RequestSetCaptureSize)[ScriptMethod = "SetCaptureSize"];
		ReflectMethod(RequestGetCaptureSize)[ScriptMethodLocked = "GetCaptureSize"];
		ReflectMethod(RequestCapture)[ScriptMethodLocked = "Capture"];
		ReflectMethod(RequestGetCapturedTexture)[ScriptMethodLocked = "GetCapturedTexture"];
	}

	return *this;
}

TShared<RayTraceComponent> RayTraceComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RayTraceComponent> rayTraceComponent = TShared<RayTraceComponent>::From(allocator->New());
	rayTraceComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return rayTraceComponent;
}

size_t RayTraceComponentModule::RequestGetCompletedPixelCount(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	return rayTraceComponent->GetCompletedPixelCount();
}

size_t RayTraceComponentModule::RequestGetTotalPixelCount(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	return rayTraceComponent->GetTotalPixelCount();
}

void RayTraceComponentModule::RequestSetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, const UShort2& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	rayTraceComponent->SetCaptureSize(engine, size);
}

void RayTraceComponentModule::RequestConfigure(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, uint16_t superSample, uint16_t tileSize, uint32_t rayCount, uint32_t bounceCount) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	rayTraceComponent->Configure(superSample, tileSize, rayCount, bounceCount);
}

UShort2 RayTraceComponentModule::RequestGetCaptureSize(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	return rayTraceComponent->GetCaptureSize();
}

void RayTraceComponentModule::RequestCapture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, IScript::Delegate<CameraComponent> cameraComponent, float averageLuminance) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_DELEGATE(cameraComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	rayTraceComponent->Capture(engine, cameraComponent.Get(), averageLuminance);
}

void RayTraceComponentModule::RequestSetOutputPath(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent, const String& outputPath) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	rayTraceComponent->SetOutputPath(outputPath);
}

TShared<TextureResource> RayTraceComponentModule::RequestGetCapturedTexture(IScript::Request& request, IScript::Delegate<RayTraceComponent> rayTraceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(rayTraceComponent);
	CHECK_THREAD_IN_MODULE(rayTraceComponent);

	return rayTraceComponent->GetCapturedTexture();
}

