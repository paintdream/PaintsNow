#define USE_SWIZZLE
#include "WidgetShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

WidgetShadingFS::WidgetShadingFS() {
	mainTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String WidgetShadingFS::GetShaderText() {
	return UnifyShaderCode(
		target = texture(mainTexture, texCoord.xy); // TODO: more on texCoord.zw
	);
}

TObject<IReflect>& WidgetShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(mainTexture)[BindInput(BindInput::MAINTEXTURE)];

		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}