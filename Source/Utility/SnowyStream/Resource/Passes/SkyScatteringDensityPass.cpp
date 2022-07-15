#include "SkyScatteringDensityPass.h"

using namespace PaintsNow;

SkyScatteringDensityPass::SkyScatteringDensityPass() {}

TObject<IReflect>& SkyScatteringDensityPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
