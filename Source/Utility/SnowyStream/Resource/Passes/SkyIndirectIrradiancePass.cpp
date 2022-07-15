#include "SkyIndirectIrradiancePass.h"

using namespace PaintsNow;

SkyIndirectIrradiancePass::SkyIndirectIrradiancePass() {}

TObject<IReflect>& SkyIndirectIrradiancePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
