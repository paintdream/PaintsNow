#include "ScreenSpaceFilterPass.h"

using namespace PaintsNow;

ScreenSpaceFilterPass::ScreenSpaceFilterPass() {}

TObject<IReflect>& ScreenSpaceFilterPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderScreen)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}