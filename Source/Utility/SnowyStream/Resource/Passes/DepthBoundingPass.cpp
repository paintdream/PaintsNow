#include "DepthBoundingPass.h"

using namespace PaintsNow;

DepthBoundingPass::DepthBoundingPass() {
}

TObject<IReflect>& DepthBoundingPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(minmax)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}