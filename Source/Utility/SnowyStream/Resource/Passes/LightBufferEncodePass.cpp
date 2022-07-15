#include "LightBufferEncodePass.h"
using namespace PaintsNow;

LightBufferEncodePass::LightBufferEncodePass() {
}

TObject<IReflect>& LightBufferEncodePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(encoder)[IShader::MetaShader(IRender::Resource::ShaderDescription::COMPUTE)];
	}

	return *this;
}