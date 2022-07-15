#include "CustomMaterialPass.h"

using namespace PaintsNow;

CustomMaterialPass::CustomMaterialPass() {}

bool CustomMaterialPass::FlushOptions() {
	bool flushed = BaseClass::FlushOptions();
	shaderParameter.description->SynchronizeInstance(shaderParameter.instanceData, *shaderTransform.description, shaderTransform.instanceData);
	return BaseClass::FlushOptions() || flushed;
}

TObject<IReflect>& CustomMaterialPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shaderTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderCompactEncode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}

void CustomMaterialPass::SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	if (stage == "VertexInstance") {
		shaderTransform.description->SetInput("Instance", type, name, value, binding, config);
	} else if (stage == "VertexInput") {
		shaderTransform.description->SetInput("Vertex", type, name, value, binding, config);
	} else if (stage == "VertexOptions") {
		shaderTransform.description->SetInput("Option", type, name, value, binding, config);
	} else if (stage == "VertexUniform") {
		shaderTransform.description->SetInput("Uniform", type, name, value, binding, config);
	} else if (stage == "VertexOutput") {
		shaderTransform.description->SetInput("Output", type, name, value, binding, config);
	} else if (stage == "VertexVarying") {
		shaderTransform.description->SetInput("Output", type, name, value, binding, config);
		shaderParameter.description->SetInput("Input", type, name, value, binding, config);
	} else if (stage == "FragmentOptions") {
		shaderParameter.description->SetInput("Option", type, name, value, binding, config);
	} else if (stage == "FragmentUniform") {
		shaderParameter.description->SetInput("Uniform", type, name, value, binding, config);
	}
}

void CustomMaterialPass::SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	if (stage == "Vertex") {
		shaderTransform.description->SetCode(code);
	} else if (stage == "Fragment") {
		shaderParameter.description->SetCode(code);
	}
}

void CustomMaterialPass::SetComplete() {
	std::vector<String> defTexturePaths;
	shaderTransform.description->SetComplete(shaderTransform.instanceData);
	shaderParameter.description->SetComplete(shaderParameter.instanceData);
}

void CustomMaterialPass::DetachDescription() {
	shaderTransform.description.Reset(new CustomMaterialDescription());
	shaderParameter.description.Reset(new CustomMaterialDescription());
}
