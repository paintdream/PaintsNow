#define USE_SWIZZLE
#include "ScreenFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenFS::ScreenFS() {
	bloomIntensity = Float3(0.5f, 0.8f, 1.2f);
	invAverageLuminance = 1.0f / 5.0f;
	
	inputColorTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture0.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture1.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture2.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	paramBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

String ScreenFS::GetShaderText() {
	return UnifyShaderCode(
		const float3x3 ACESInputMat = make_float3x3(
			0.59719, 0.07600, 0.02840,
			0.35458, 0.90834, 0.13383,
			0.04823, 0.01566, 0.83777
		);

		const float3x3 ACESOutputMat = make_float3x3(
			1.60475, -0.10208, -0.00327,
			-0.53108, 1.10813, -0.07276,
			-0.07367, -0.00605, 1.07602
		);

		float4 bloomColor = float4(0, 0, 0, 0);
		bloomColor += texture(inputBloomTexture0, rasterCoord) * bloomIntensity.x;
		bloomColor += texture(inputBloomTexture1, rasterCoord) * bloomIntensity.y;
		bloomColor += texture(inputBloomTexture2, rasterCoord) * bloomIntensity.z;

		float3 color = texture(inputColorTexture, rasterCoord).xyz;
		const float A = 2.51f;
		const float B = 0.03f;
		const float C = 2.43f;
		const float D = 0.59f;
		const float E = 0.14f;
		color = max(mult_vec(ACESInputMat, (color + bloomColor.xyz) * invAverageLuminance), float3(0.0, 0.0, 0.0));
		color = color * (color * A + float3(B, B, B)) / (color * (color * C + float3(D, D, D)) + float3(E, E, E));
		outputColor.xyz = saturate(mult_vec(ACESOutputMat, color));
		outputColor.xyz = pow(outputColor.xyz, float3(1.0 / GAMMA, 1.0 / GAMMA, 1.0 / GAMMA));
		outputColor.w = 1;
	);
}

TObject<IReflect>& ScreenFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(inputColorTexture);
		ReflectProperty(inputBloomTexture0);
		ReflectProperty(inputBloomTexture1);
		ReflectProperty(inputBloomTexture2);
		ReflectProperty(paramBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];

		ReflectProperty(bloomIntensity)[paramBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invAverageLuminance)[paramBuffer][BindInput(BindInput::GENERAL)];
	}

	return *this;
}
