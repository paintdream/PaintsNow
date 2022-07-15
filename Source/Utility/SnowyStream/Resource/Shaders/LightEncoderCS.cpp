#define USE_SWIZZLE
#include "LightEncoderCS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

LightEncoderCS::LightEncoderCS() : computeGroup(8, 8, 1) {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	depthTexture.description.state.format = IRender::Resource::TextureDescription::HALF;
	depthTexture.description.state.layout = IRender::Resource::TextureDescription::RG;
	depthTexture.memorySpec = IShader::MEMORY_READONLY;
	lightInfoBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT);
	lightBuffer.description.state.usage = IRender::Resource::BufferDescription::STORAGE; // SSBO
	lightBuffer.memorySpec = IShader::MEMORY_WRITEONLY;
}

String LightEncoderCS::GetShaderText() {
	return UnifyShaderCode(
		uint3 id = WorkGroupID * WorkGroupSize + LocalInvocationID;
		float2 depthRange = imageLoad(depthTexture, int2(id.x, id.y)).xy;
		float2 rasterCoord = float2((float(id.x) + 0.5) * invScreenSize.x, (float(id.y) + 0.5) * invScreenSize.y);
		float3 farPosition = unprojection(inverseProjectionParams, float3(rasterCoord.x, rasterCoord.y, depthRange.x) * float(2) - float3(1.0, 1.0, 1.0));
		float3 nearPosition = unprojection(inverseProjectionParams, float3(rasterCoord.x, rasterCoord.y, depthRange.y) * float(2.0) - float3(1.0, 1.0, 1.0));
		// if (id.y < WorkGroupSize.y * NumWorkGroups.y / 2)
		{
			uint offset = (id.x + id.y * NumWorkGroups.x * WorkGroupSize.x) * 64;
			uint count = min(uint(lightCount), uint(255));
			uint packedLightID = 0;
			uint j = 0;
			for (uint i = 0; i < count; i++) {
				float4 lightInfo = lightInfos[i];
				if (lightInfo.w == 0.0) {
					packedLightID = (packedLightID << 8) + (i + 1);
					j++;
				} else {
					float3 L = lightInfo.xyz - nearPosition.xyz;
					float3 F = lightInfo.xyz - farPosition.xyz;
					float3 P = normalize(farPosition.xyz - nearPosition.xyz);
					float PoL = dot(P, L);
					float PoF = dot(P, F);
					float r = dot(L, L) - PoL * PoL;
					if (r <= lightInfo.w && (PoL * PoF <= 0 || dot(L, L) <= lightInfo.w || dot(F, F) <= lightInfo.w)) {
						packedLightID = (packedLightID << 8) + (i + 1);
						j++;
					}
				}

				if (j == 4) {
					// commit
					lightBufferData[offset++] = packedLightID;
					packedLightID = 0;
					j = 0;
				}
			}

			// last commit
			lightBufferData[offset++] = packedLightID;
		}
		/*
		lightBufferData[offset++] = id.x;
		lightBufferData[offset++] = id.y;
		lightBufferData[offset++] = id.z;
		lightBufferData[offset++] = NumWorkGroups.x;
		lightBufferData[offset++] = NumWorkGroups.y;
		lightBufferData[offset++] = WorkGroupSize.x;
		lightBufferData[offset++] = WorkGroupSize.y;*/
		/*
		lightBufferData[offset++] = 1 + (2 << 8) + (3 << 16) + (4 << 24);
		lightBufferData[offset++] = 5 + (6 << 8) + (7 << 16) + (8 << 24);
		lightBufferData[offset++] = 9 + (10 << 8) + (11 << 16) + (12 << 24);*/
	);
}

TObject<IReflect>& LightEncoderCS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(computeGroup)[BindInput(BindInput::COMPUTE_GROUP)];
		ReflectProperty(depthTexture);
		ReflectProperty(lightInfoBuffer);
		ReflectProperty(lightBuffer);

		ReflectProperty(inverseProjectionParams)[lightInfoBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(invScreenSize)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightCount)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reserved)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightInfoBuffer][BindInput(BindInput::GENERAL)];

		ReflectProperty(lightBufferData)[lightBuffer][BindInput(BindInput::GENERAL)];
	}

	return *this;
}
