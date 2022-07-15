#define USE_SWIZZLE
#include "StandardLightingTextureEncodedFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

StandardLightingTextureEncodedFS::StandardLightingTextureEncodedFS() : lightCount(0), cubeLevelInv(0), cubeStrength(1.0f) {
	specTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D_CUBE;
	lightInfoBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
	paramBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT * 2);
}

String StandardLightingTextureEncodedFS::GetShaderText() {
	return UnifyShaderCode(
	baseColor = pow(baseColor, float3(GAMMA, GAMMA, GAMMA));
	float3 diff = (baseColor - baseColor * metallic) / PI;
	float3 spec = lerp(float3(0.04, 0.04, 0.04), baseColor, metallic);
	float3 V = normalize(float3(0, 0, 1) - viewPosition);
	float3 N = viewNormal;
	float NoV = saturate(dot(V, N));

	float3 R = mult_vec(float3x3(invWorldNormalMatrix), reflect(V, N));
	float3 env = screenSpaceColor;
	if (env.x < 0) {
		env = textureLod(specTexture, R, roughness * cubeLevelInv).xyz * cubeStrength;
	}

	float4 r = float4(-1, -0.0275, -0.572, 0.022) * roughness + float4(1, 0.0425, 1.04, -0.04);
	float a = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a + r.zw;
	env = env * AB.x + AB.y;
	mainColor = float4(0, 0, 0, 1);
	// mainColor.xyz += diff.xyz * float3(1.0, 1.0, 1.0); // ambient
	mainColor.xyz = mainColor.xyz + env * spec;

	float4 idx = texture(lightTexture, rasterCoord.xy) * float(255.0);
	float f = saturate(50.0 * spec.y);
	float p = roughness * roughness;
	p = p * p;

	float4 NoH = float4(0, 0, 0, 0);
	float4 NoL = float4(0, 0, 0, 0);
	float4 VoH = float4(0, 0, 0, 0);
	float3 lightColor[4];

	int k;
	for (k = 0; k < 4; k++) {
		if (idx[k] < 0.5) { break; }
		int i = int(round(idx[k])); // Intel Graphics old driver bug, must use round

		float4 pos = lightInfos[i * 2 - 2];
		float4 color = lightInfos[i * 2 - 1];

		float nondirectional = step(0.025, pos.w);
		float3 L = pos.xyz - viewPosition.xyz * nondirectional;
		float dist = dot(L, L) * nondirectional;
		float falloff = saturate(dist / max(0.025, pos.w));
		float s = (1.0 - falloff * falloff) / (1.0 + dist * color.w) * step(-0.5, nondirectional - shadow);
		L = normalize(L);
		float3 H = normalize(L + V);

		lightColor[k] = color.xyz * s;
		NoH[k] = saturate(dot(N, H));
		NoL[k] = saturate(dot(N, L));
		VoH[k] = saturate(dot(V, H));
	}

	float4 q = (NoH.xyzw * p - NoH.xyzw) * NoH.xyzw + float4(1.0, 1.0, 1.0, 1.0);
	float4 vl = clamp(NoL.xyzw, float4(0.1, 0.1, 0.1, 0.1), float4(1, 1, 1, 1));
	float vlc = clamp(NoV, float(0.01), float(1.0));
	float4 vls = vl.xyzw * sqrt(saturate(-vlc * p + vlc) * vlc + p);
	vls = vls + sqrt(saturate(-vl.xyzw * p + vl.xyzw) * vl.xyzw + p) * vlc;
	float4 DG = (float4(0.5, 0.5, 0.5, 0.5) / PI * p) / max(vls.xyzw * (q * q), float4(0.0001, 0.0001, 0.0001, 0.0001));
	float4 e = exp2(VoH.xyzw * (VoH.xyzw * float(-5.55473) - float4(6.98316, 6.98316, 6.98316, 6.98316)));

	for (int i = 0; i < k; i++) {
		float3 F = spec + (float3(f, f, f) - spec) * e[i];
		mainColor.xyz = mainColor.xyz + (diff + F * DG[i]) * lightColor[i].xyz * NoL[i];
	}

	// mainColor.xyz = mainColor.xyz * float(0.0001) + float3(1 - shadow, 1 - shadow, 1 - shadow);
);
}

TObject<IReflect>& StandardLightingTextureEncodedFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(lightTexture);
		ReflectProperty(specTexture);
		ReflectProperty(lightInfoBuffer);
		ReflectProperty(paramBuffer);

		ReflectProperty(screenSpaceColor)[BindInput(BindInput::LOCAL)];
		ReflectProperty(viewPosition)[BindInput(BindInput::LOCAL)];
		ReflectProperty(viewNormal)[BindInput(BindInput::LOCAL)];
		ReflectProperty(baseColor)[BindInput(BindInput::LOCAL)];
		ReflectProperty(metallic)[BindInput(BindInput::LOCAL)];
		ReflectProperty(roughness)[BindInput(BindInput::LOCAL)];
		ReflectProperty(shadow)[BindInput(BindInput::LOCAL)];
		ReflectProperty(occlusion)[BindInput(BindInput::LOCAL)];
		ReflectProperty(invWorldNormalMatrix)[paramBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(cubeLevelInv)[paramBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(cubeStrength)[paramBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(mainColor)[BindOutput(BindOutput::COLOR)];
	}
	
	return *this;
}
