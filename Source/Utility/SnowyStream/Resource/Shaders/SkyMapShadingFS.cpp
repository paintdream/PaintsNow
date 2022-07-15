#define USE_SWIZZLE
#include "SkyMapShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyMapShadingFS::SkyMapShadingFS() {
	skyMapTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D_CUBE;
}

String SkyMapShadingFS::GetShaderText() {
	return UnifyShaderCode(
		float3 N = normalize(worldPosition.xyz);
		outputColor = texture(skyMapTexture, N, float(0));
	);
}

TObject<IReflect>& SkyMapShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(skyMapTexture)[BindInput(BindInput::MAINTEXTURE)];
		ReflectProperty(worldPosition)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}