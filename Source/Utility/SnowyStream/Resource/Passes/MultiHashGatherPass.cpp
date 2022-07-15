#include "MultiHashGatherPass.h"

using namespace PaintsNow;

MultiHashGatherPass::MultiHashGatherPass() {}

TObject<IReflect>& MultiHashGatherPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderMultiHashGather)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}