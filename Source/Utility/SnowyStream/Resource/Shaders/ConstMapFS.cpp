#define USE_SWIZZLE
#include "ConstMapFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ConstMapFS::ConstMapFS() : enableBaseColorTexture(false), enableBaseColorTint(false), enableAlphaTest(false) {
}

String ConstMapFS::GetShaderText() {
	return UnifyShaderCode(
		target = tintColor;

		if (enableBaseColorTexture) {
			float4 color = texture(baseColorTexture, texCoord.xy);
			if (enableAlphaTest) {
				clip(color.w - 0.5);
			}

			if (enableBaseColorTint) {
				target.xyz = target.xyz * color.xyz;
			}
		}
	);
}

TObject<IReflect>& ConstMapFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(baseColorTexture)[BindInput(BindInput::MAINTEXTURE)];
		ReflectProperty(enableBaseColorTexture)[BindConst<bool>(baseColorTexture)];
		ReflectProperty(enableBaseColorTint)[BindConst<bool>(enableBaseColorTint)];
		ReflectProperty(enableAlphaTest)[BindConst<bool>(enableAlphaTest)];

		ReflectProperty(tintColor)[BindInput(BindInput::GENERAL)];
		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}