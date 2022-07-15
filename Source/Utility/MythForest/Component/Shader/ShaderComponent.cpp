#include "ShaderComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include <sstream>

using namespace PaintsNow;

ShaderComponent::ShaderComponent(const TShared<ShaderResource>& shader, const String& n) : name(n) {
	customMaterialShader = shader->QueryInterface(UniqueType<ShaderResourceImpl<CustomMaterialPass> >());
	assert(customMaterialShader); // by now we only support CustomMaterialPass.
	customMaterialShader.Reset(static_cast<ShaderResourceImpl<CustomMaterialPass>*>(customMaterialShader->Clone()));
	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.DetachDescription();
	std::stringstream ss;
	ss << std::hex << (void*)this;

	customMaterialShader->SetLocation(shader->GetLocation() + "/" + name + "/" + StdToUtf8(ss.str()));
}

ShaderComponent::~ShaderComponent() {}

void ShaderComponent::SetInput(Engine& engine, const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetInput(stage, type, name, value, binding, config);
}

void ShaderComponent::SetCode(Engine& engine, const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	assert(customMaterialShader);

	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetCode(stage, code, config);
}

void ShaderComponent::SetComplete(Engine& engine, IScript::Request::Ref callback) {
	assert(customMaterialShader);
	CustomMaterialPass& pass = static_cast<CustomMaterialPass&>(customMaterialShader->GetPass());
	pass.SetComplete();

	// Try to compile
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();

	ReferenceObject();

	PassBase::Updater& updater = customMaterialShader->GetPassUpdater();
	updater.Initialize(pass);

	// fill vertex buffers
	pass.Compile(render, queue, customMaterialShader->GetShaderResource(), Wrap(this, &ShaderComponent::OnShaderCompiled), &engine, (void*)callback.value);
}

void ShaderComponent::OnShaderCompiled(IRender::Resource* resource, IRender::Resource::ShaderDescription& desc, IRender::Resource::ShaderDescription::Stage stage, const String& info, const String& shaderCode) {
	if (stage == IRender::Resource::ShaderDescription::END) {
		Engine* engine = reinterpret_cast<Engine*>(desc.context);
		assert(engine != nullptr);

		IScript::Request::Ref compileCallbackRef;
		compileCallbackRef.value = reinterpret_cast<size_t>(desc.instance);
		if (compileCallbackRef) {
			engine->bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(compileCallbackRef, info, shaderCode));
		}

#ifdef _DEBUG
		engine->interfaces.render.SetResourceNote(resource, customMaterialShader->GetLocation());
#endif

		customMaterialShader->SetShaderResource(resource);
		if (customMaterialShader->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_ORPHAN) {
			ResourceManager& resourceManager = customMaterialShader->GetResourceManager();
			SharedLockGuardWriter guard(resourceManager.GetThreadApi(), resourceManager.GetLock());
			resourceManager.Insert(customMaterialShader());
		}

		ReleaseObject();
	}
}

TShared<MaterialResource> ShaderComponent::ExportMaterial(Engine& engine, const TShared<MaterialResource>& materialTemplate) {
	// create anouymous material
	TShared<MaterialResource> materialResource = engine.snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);

	assert(materialTemplate->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_UPLOADED);
	assert(!materialTemplate->materialParams.variables.empty());

	materialResource->materialParams = materialTemplate->materialParams;
	materialResource->textureResources = materialTemplate->textureResources;
	materialResource->originalShaderResource = customMaterialShader();

	return materialResource;
}
