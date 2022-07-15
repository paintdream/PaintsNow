#include "SkyDirectIrradiancePass.h"

using namespace PaintsNow;

SkyDirectIrradiancePass::SkyDirectIrradiancePass() {}

TObject<IReflect>& SkyDirectIrradiancePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
