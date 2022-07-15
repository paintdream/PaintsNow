#define USE_SWIZZLE
#include "MultiHashTraceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashTraceFS::MultiHashTraceFS() : sigma(1) {
	srcDepthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	srcLitTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	dstDepthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	dstNormalRoughnessMetallicTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;

	offsets.resize(16);
}

TObject<IReflect>& MultiHashTraceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(srcDepthTexture);
		ReflectProperty(srcLitTexture);
		ReflectProperty(dstDepthTexture);
		ReflectProperty(dstBaseColorOcclusionTexture);
		ReflectProperty(dstNormalRoughnessMetallicTexture);

		ReflectProperty(uniformBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(dstInverseProjection)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(srcProjection)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(srcInverseProjection)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(offsets)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(srcOrigin)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(sigma)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(dstLit)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}

String MultiHashTraceFS::GetShaderText() {
	return UnifyShaderCode(
	float dstDepth = textureLod(dstDepthTexture, rasterCoord.xy, float(0)).x;
	float4 dstPos = float4(rasterCoord.x, rasterCoord.y, dstDepth, 1);
	dstPos.xyz = dstPos.xyz * float3(2, 2, 2) - float3(1, 1, 1);
	dstPos = mult_vec(dstInverseProjection, dstPos);
	float4 srcCoord = mult_vec(srcProjection, dstPos);
	dstPos.xyz = dstPos.xyz / dstPos.w;
	srcCoord.xy = srcCoord.xy / srcCoord.z * float2(0.5, 0.5) + float2(0.5, 0.5);
	float4 dstColor = textureLod(dstBaseColorOcclusionTexture, rasterCoord.xy, float(0));
	float4 dstInfo = textureLod(dstNormalRoughnessMetallicTexture, rasterCoord.xy, float(0));
	const float3 MINSPEC = float3(0.04, 0.04, 0.04);
	dstColor.xyz = pow(dstColor.xyz, float3(GAMMA, GAMMA, GAMMA));
	float3 N;
	N.xy = dstInfo.xy * float2(2, 2) - float2(1, 1);
	N.z = sqrt(max(0.0, dot(N.xy, N.xy)));
	float3 V = -normalize(dstPos.xyz);
	float NoV = dot(V, N);
	float roughness = dstInfo.z;
	float metallic = dstInfo.w;
	float3 diff = (dstColor.xyz - dstColor.xyz * metallic) / PI;
	float3 spec = lerp(MINSPEC, dstColor.xyz, metallic);
	float p = roughness * roughness;
	p = p * p;
	dstLit = float4(0, 0, 0, 0.0001);
	for (int i = 0; i < 16; i++) {
		float2 biasCoord = srcCoord.xy + offsets[i];
		float srcDepth = textureLod(srcDepthTexture, biasCoord, float(0)).x;
		float4 srcPos = float4(srcCoord.x, srcCoord.y, srcDepth, 1);
		srcPos.xyz = srcPos.xyz * float3(2, 2, 2) - float3(1, 1, 1);
		srcPos = mult_vec(srcInverseProjection, srcPos);
		srcPos.xyz = srcPos.xyz / srcPos.w;
		float3 L = srcPos.xyz - dstPos.xyz;
		float dist = length(L) + 0.0001; L /= dist;
		float NoL = dot(L, N);
		float3 I = normalize(srcOrigin - srcPos.xyz);
		dist = dist / max(0.0001, dot(I, L));
		float s = exp2(-dist * dist * sigma);
		dstLit.w += s;
		if (NoL < 0 && s < 0.0001) { break; }
		float4 srcLit = textureLod(srcLitTexture, biasCoord, float(0));
		float3 H = normalize(N + V);
		float NoH = dot(N, H);
		float VoH = dot(V, H);
		float q = (NoH * p - NoH) * NoH + 1.0;
		float2 vl = clamp(float2(NoV, NoL), float2(0.01, 0.1), float2(1, 1));
		vl = vl.yx * sqrt(saturate(-vl * p + vl) * vl + p);
		float DG = p / max(q * q, 0.0001) * (0.5 / PI) / max(vl.x + vl.y, 0.0001);
		float f = saturate(50.0 * spec.y);
		float3 F = spec + (float3(f, f, f) - spec) * float(exp2(VoH * (-5.55473 * VoH - 6.98316)));
		dstLit.xyz = dstLit.xyz + (diff + F * DG) * srcLit.xyz / srcLit.w * NoL * s;
	});
}
