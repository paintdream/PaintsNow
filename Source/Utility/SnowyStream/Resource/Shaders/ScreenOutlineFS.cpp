#define USE_SWIZZLE
#include "ScreenOutlineFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenOutlineFS::ScreenOutlineFS() {
	inputMaskTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	paramBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenOutlineFS::GetShaderText() {
	return UnifyShaderCode(
		outputColor = texture(inputMaskTexture, rasterCoord);
	);
}

TObject<IReflect>& ScreenOutlineFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(inputMaskTexture);
		ReflectProperty(paramBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
