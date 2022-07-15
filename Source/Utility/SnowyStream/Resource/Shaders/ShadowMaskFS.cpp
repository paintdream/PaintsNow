#define USE_SWIZZLE
#include "ShadowMaskFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ShadowMaskFS::ShadowMaskFS() : shadowBias(0.001f) {
	uniformBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
	shadowTexture.description.state.pcf = true;
}

String ShadowMaskFS::GetShaderText() {
	return UnifyShaderCode(
	rasterCoord.xy = rasterCoord.xy * invScreenSize.xy;
	float depth = textureLod(depthTexture, rasterCoord.xy, float(0)).x;
	float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
	position = position * float(2) - float4(1, 1, 1, 1);
	position = mult_vec(reprojectionMatrix, position);
	position.xyz = position.xyz / position.w;
	float3 uv = abs(position.xyz);
	clip(float(uv.x < 1 && uv.y < 1 && uv.z < 1) - float(0.5));

	position.z = position.z + shadowBias;
	position.xyz = position.xyz * float(0.5) + float3(0.5 - unjitter.x, 0.5 - unjitter.y, 0.5);
	shadow.x = textureShadow(shadowTexture, position.xyz); // hardware pcf
	);
}

TObject<IReflect>& ShadowMaskFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shadowTexture);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(reprojectionMatrix)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(unjitter)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(shadowBias)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(shadow)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
