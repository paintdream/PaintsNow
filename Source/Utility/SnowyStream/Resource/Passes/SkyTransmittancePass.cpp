#include "SkyTransmittancePass.h"

using namespace PaintsNow;

SkyTransmittancePass::SkyTransmittancePass() {
	transform.enableRasterCoord = false;
	transform.enableVertexTransform = false;
}

TObject<IReflect>& SkyTransmittancePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
