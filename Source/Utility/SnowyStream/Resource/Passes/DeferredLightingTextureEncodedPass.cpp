#include "DeferredLightingTextureEncodedPass.h"

using namespace PaintsNow;

DeferredLightingTextureEncodedPass::DeferredLightingTextureEncodedPass() {}

TObject<IReflect>& DeferredLightingTextureEncodedPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(deferredCompactDecode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(standardLighting)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}