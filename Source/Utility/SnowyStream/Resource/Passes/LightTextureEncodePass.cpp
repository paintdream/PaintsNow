#include "LightTextureEncodePass.h"

using namespace PaintsNow;

LightTextureEncodePass::LightTextureEncodePass() {
}

TObject<IReflect>& LightTextureEncodePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(encoder)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}