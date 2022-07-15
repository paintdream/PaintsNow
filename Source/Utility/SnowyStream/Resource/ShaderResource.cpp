#include "ShaderResource.h"
#include "../ResourceManager.h"
#include "../Manager/RenderResourceManager.h"
#include "../../../General/Interface/IShader.h"

using namespace PaintsNow;

ShaderResource::ShaderResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), shaderResource(nullptr) {}

ShaderResource::~ShaderResource() {
	if (shaderResource != nullptr) { // orphan?
		DeviceResourceManager<IRender>& manager = static_cast<DeviceResourceManager<IRender>&>(resourceManager);
		manager.GetDevice().DeleteResource(static_cast<IRender::Queue*>(resourceManager.GetContext()), shaderResource);
	}
}

const String& ShaderResource::GetShaderName() const {
	return GetUnique()->GetName();
}

bool ShaderResource::operator << (IStreamBase& stream) {
	return false;
}

bool ShaderResource::operator >> (IStreamBase& stream) const {
	return false;
}

const Bytes& ShaderResource::GetHashValue() const {
	return hashValue;
}

void ShaderResource::OnResourceUploaded(IRender& render, IRender::Queue* queue) {

}

void ShaderResource::Attach(IRender& render, void* deviceContext) {
	// compute hash value
	PassBase& pass = GetPass();
	hashValue = pass.ExportHash();

	if (shaderResource != nullptr) return; // already attached.

	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	Compile(render, queue);

#ifdef _DEBUG
	render.SetResourceNote(shaderResource, GetLocation());
#endif

	RenderResourceBase::Upload(render, deviceContext);
}

void ShaderResource::Compile(IRender& render, IRender::Queue* queue, const Bytes* newHash) {
	PassBase& pass = GetPass();
	// compile default shader
	if (newHash != nullptr) {
		hashValue = *newHash;
	} else {
		hashValue = pass.ExportHash();
	}

	shaderResource = pass.Compile(render, queue, shaderResource);
}

IRender::Resource* ShaderResource::GetShaderResource() const {
	return shaderResource;
}

void ShaderResource::SetShaderResource(IRender::Resource* res) {
	shaderResource = res;
}

bool ShaderResource::Complete(size_t runtimeVersion) {
	// printf("Variant Shader Completed: %s\n", GetLocation().c_str());
	return BaseClass::Complete(runtimeVersion);
}

void ShaderResource::Detach(IRender& render, void* deviceContext) {
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	if (shaderResource != nullptr) {
		render.DeleteResource(queue, shaderResource);
		shaderResource = nullptr;
	}
}

IReflectObject* ShaderResource::Clone() const {
	ShaderResource* clone = new ShaderResource(resourceManager, uniqueLocation);
	clone->shaderResource = nullptr;
	clone->Flag().fetch_and(~RESOURCE_UPLOADED, std::memory_order_relaxed);
	clone->Flag().fetch_or(RESOURCE_ORPHAN, std::memory_order_relaxed);

	return clone;
}

void ShaderResource::Upload(IRender& render, void* deviceContext) {

}

void ShaderResource::Download(IRender& render, void* deviceContext) {

}

const String& ShaderResource::GetShaderPathPrefix() {
	static const String shaderPrefix = "[Runtime]/ShaderResource/";
	return shaderPrefix;
}

PassBase& ShaderResource::GetPass() {
	static PassBase dummy;
	assert(false);
	return dummy;
}

PassBase::Updater& ShaderResource::GetPassUpdater() {
	static PassBase::Updater updater;
	assert(false);
	return updater;
}

TObject<IReflect>& ShaderResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
