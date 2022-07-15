#include "ScreenSpaceTracePass.h"

using namespace PaintsNow;

ScreenSpaceTracePass::ScreenSpaceTracePass() {}

TObject<IReflect>& ScreenSpaceTracePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderScreen)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}