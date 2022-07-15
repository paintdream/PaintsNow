#define USE_SWIZZLE
#include "MultiHashSetupFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashSetupFS::MultiHashSetupFS() {
	setupParamBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
	lightDepthTexture.description.state.pcf = true;
}

TObject<IReflect>& MultiHashSetupFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(setupParamBuffer);
		ReflectProperty(noiseTexture);
		ReflectProperty(lightDepthTexture);

		ReflectProperty(lightReprojectionMatrix)[setupParamBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightColor)[setupParamBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightPosition)[setupParamBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[setupParamBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(tintColor)[BindInput(BindInput::LOCAL)];
		ReflectProperty(outputIrradiance)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}

String MultiHashSetupFS::GetShaderText() {
	return UnifyShaderCode(
		float noise = texture(noiseTexture, rasterCoord.xy * tintColor.xy + tintColor.zw).x;
		clip(noise - 0.5);
		float2 uv = rasterCoord.xy * invScreenSize;
		float4 position = float4(uv.x, uv.y, rasterCoord.z, 1);
		position = position * float(2) - float4(1, 1, 1, 1);
		position = mult_vec(lightReprojectionMatrix, position);
		position.xyz = position.xyz / position.w;
		position.xyz = position.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
		float shadow = textureShadow(lightDepthTexture, position.xyz);
		// by now we only suport directional light
		outputIrradiance = lightColor - lightColor * shadow;
	);
}