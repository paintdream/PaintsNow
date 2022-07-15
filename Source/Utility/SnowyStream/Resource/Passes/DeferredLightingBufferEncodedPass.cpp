#include "DeferredLightingBufferEncodedPass.h"

using namespace PaintsNow;

DeferredLightingBufferEncodedPass::DeferredLightingBufferEncodedPass() {}

TObject<IReflect>& DeferredLightingBufferEncodedPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(deferredCompactDecode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(standardLighting)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}