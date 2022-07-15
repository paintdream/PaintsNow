#include "DepthResolvePass.h"

using namespace PaintsNow;

DepthResolvePass::DepthResolvePass() {
}

TObject<IReflect>& DepthResolvePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(resolve)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}