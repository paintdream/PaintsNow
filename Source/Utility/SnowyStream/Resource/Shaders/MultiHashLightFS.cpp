#define USE_SWIZZLE
#include "MultiHashLightFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashLightFS::MultiHashLightFS() {
	
}

TObject<IReflect>& MultiHashLightFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(depthTexture);
		ReflectProperty(lightDepthTexture);
		ReflectProperty(lightParamBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(invProjectionMatrix)[BindInput(BindInput::GENERAL)];
		ReflectProperty(lightProjectionMatrix)[BindInput(BindInput::GENERAL)];
		ReflectProperty(lightColor)[lightParamBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightAttenuation)[lightParamBuffer][BindInput(BindInput::GENERAL)];

		ReflectProperty(blendColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}

String MultiHashLightFS::GetShaderText() {
	return UnifyShaderCode(
		float depth = texture(depthTexture, rasterCoord).x;
		float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
		position = mult_vec(invProjectionMatrix, position);
		position.xyz = position.xyz / position.w;

		float4 refPosition = mult_vec(lightProjectionMatrix, position);
		refPosition.xyz = refPosition.xyz / refPosition.w * float3(0.5, 0.5, 0.5) + float3(0.5, 0.5, 0.5);
		// query shadow info
		float refDepth = texture(lightDepthTexture, refPosition.xy).x;
		clip(refDepth - refPosition.w);

		// TODO: lit
		blendColor.xyz = lightColor;
	);
}
