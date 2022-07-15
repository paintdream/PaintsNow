#include "ForwardLightingPass.h"

using namespace PaintsNow;

ForwardLightingPass::ForwardLightingPass() {
	standardTransform.enableViewPosition = true;
}

TObject<IReflect>& ForwardLightingPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(standardTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(standardParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(standardLighting)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}