#define USE_SWIZZLE
#include "BloomFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

BloomFS::BloomFS() : colorThreshold(0.5f, 0.5f, 0.5f), colorScale(1) {
	screenTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBloomBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String BloomFS::GetShaderText() {
	return UnifyShaderCode(
		float4 a = texture(screenTexture, rasterCoord + float2(0.5, 1.5) * invScreenSize);
		float4 b = texture(screenTexture, rasterCoord + float2(-1.5, 0.5) * invScreenSize);
		float4 c = texture(screenTexture, rasterCoord + float2(-0.5, -1.5) * invScreenSize);
		float4 d = texture(screenTexture, rasterCoord + float2(1.5, -0.5) * invScreenSize);
		float4 v = (a + b + c + d) * (colorScale * float(0.25));
		v.xyz = v.xyz - colorThreshold;
		outputColor = max(v, float4(0, 0, 0, 0));
	);
}

TObject<IReflect>& BloomFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTexture);
		ReflectProperty(uniformBloomBuffer);

		ReflectProperty(colorThreshold)[uniformBloomBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(colorScale)[uniformBloomBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[uniformBloomBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
