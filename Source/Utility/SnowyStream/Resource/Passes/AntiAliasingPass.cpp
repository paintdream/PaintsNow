#include "AntiAliasingPass.h"

using namespace PaintsNow;

AntiAliasingPass::AntiAliasingPass() {
}

TObject<IReflect>& AntiAliasingPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(antiAliasing)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}