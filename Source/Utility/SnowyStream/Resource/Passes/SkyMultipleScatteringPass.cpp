#include "SkyMultipleScatteringPass.h"

using namespace PaintsNow;

SkyMultipleScatteringPass::SkyMultipleScatteringPass() {}

TObject<IReflect>& SkyMultipleScatteringPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
