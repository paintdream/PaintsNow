#include "SkySingleScatteringPass.h"

using namespace PaintsNow;

SkySingleScatteringPass::SkySingleScatteringPass() {}

TObject<IReflect>& SkySingleScatteringPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
