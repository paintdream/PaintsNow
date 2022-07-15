#include "StandardPass.h"

using namespace PaintsNow;

StandardPass::StandardPass() {}

TObject<IReflect>& StandardPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(standardTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderCompactEncode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}