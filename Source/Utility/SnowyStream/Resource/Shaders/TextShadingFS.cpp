#define USE_SWIZZLE
#include "TextShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

TextShadingFS::TextShadingFS() {
	mainTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String TextShadingFS::GetShaderText() {
	return UnifyShaderCode(
		target.w = texture(mainTexture, texCoord.xy).x * color.w;
		target.xyz = color.xyz * target.w; // pre-multiplied
	);
}

TObject<IReflect>& TextShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(mainTexture)[BindInput(BindInput::MAINTEXTURE)];
		ReflectProperty(color)[BindInput(BindInput::COLOR)];
		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}