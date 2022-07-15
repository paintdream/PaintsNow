#include "MultiHashSetupPass.h"

using namespace PaintsNow;

MultiHashSetupPass::MultiHashSetupPass() {
	standardTransform.enableViewProjectionMatrix = true;
	standardTransform.enableVertexNormal = true;
	standardTransform.enableVertexColor = false;
	standardTransform.enableVertexTangent = true;
	standardTransform.enableInstancedColor = true;

	shaderParameter.enableBaseColorTint = false;
}

TObject<IReflect>& MultiHashSetupPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(standardTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderMultiHashSetup)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderCompactEncode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}