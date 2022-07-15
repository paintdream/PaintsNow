#include "ScreenOutlinePass.h"

using namespace PaintsNow;

ScreenOutlinePass::ScreenOutlinePass() {}

TObject<IReflect>& ScreenOutlinePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderScreen)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}