#define USE_SWIZZLE
#include "DeferredCompactFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

DeferredCompactEncodeFS::DeferredCompactEncodeFS() {
}

String DeferredCompactEncodeFS::GetShaderText() {
	return UnifyShaderCode(
		encodeNormalRoughnessMetallic.xy = outputNormal.xy * float(127.0 / 255.0) + float2(128.0 / 255.0, 128.0 / 255.0);
		encodeNormalRoughnessMetallic.z = roughness;
		encodeNormalRoughnessMetallic.w = metallic;
		encodeBaseColorOcclusion.xyz = outputColor.xyz;
		encodeBaseColorOcclusion.w = occlusion;
	);
}

TObject<IReflect>& DeferredCompactEncodeFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(outputColor)[BindInput(BindInput::LOCAL)];
		ReflectProperty(outputNormal)[BindInput(BindInput::LOCAL)];
		ReflectProperty(occlusion)[BindInput(BindInput::LOCAL)];
		ReflectProperty(metallic)[BindInput(BindInput::LOCAL)];
		ReflectProperty(roughness)[BindInput(BindInput::LOCAL)];

		ReflectProperty(encodeBaseColorOcclusion)[BindOutput(BindOutput::COLOR)];
		ReflectProperty(encodeNormalRoughnessMetallic)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}

DeferredCompactDecodeFS::DeferredCompactDecodeFS() : enableScreenSpaceColor(true) {
	BaseColorOcclusionTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	NormalRoughnessMetallicTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	ScreenTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	ReflectCoordTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	DepthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformProjectionBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String DeferredCompactDecodeFS::GetShaderText() {
	return UnifyShaderCode(
		depth = textureLod(DepthTexture, rasterCoord.xy, float(0)).x;
		float4 NormalRoughnessMetallic = textureLod(NormalRoughnessMetallicTexture, rasterCoord.xy, float(0));
		float4 BaseColorOcclusion = textureLod(BaseColorOcclusionTexture, rasterCoord.xy, float(0));
		shadow = textureLod(ShadowTexture, rasterCoord.xy, float(0)).x;

		float2 nn = NormalRoughnessMetallic.xy * float(255.0 / 127.0) - float2(128.0 / 127.0, 128.0 / 127.0);
		viewNormal = float3(nn.x, nn.y, sqrt(max(0.0, 1 - dot(nn.xy, nn.xy))));
		roughness = NormalRoughnessMetallic.z;
		metallic = NormalRoughnessMetallic.w;

		baseColor = BaseColorOcclusion.xyz;
		occlusion = BaseColorOcclusion.w;

		float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
		position.xyz = position.xyz * float3(2, 2, 2) - float3(1, 1, 1);
		position = mult_vec(inverseProjectionMatrix, position);
		viewPosition = position.xyz / position.w;

		screenSpaceColor = float3(-1, -1, -1);
		if (enableScreenSpaceColor) {
			float2 coord = textureLod(ReflectCoordTexture, rasterCoord.xy, float(0)).xy;
			if (coord.x + coord.y > 0) {
				screenSpaceColor = textureLod(ScreenTexture, coord.xy, float(0)).xyz;
			}
		}
	);
}

TObject<IReflect>& DeferredCompactDecodeFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(BaseColorOcclusionTexture);
		ReflectProperty(NormalRoughnessMetallicTexture);
		ReflectProperty(DepthTexture);
		ReflectProperty(ShadowTexture);
		ReflectProperty(ScreenTexture);
		ReflectProperty(ReflectCoordTexture);
		ReflectProperty(uniformProjectionBuffer);
		ReflectProperty(enableScreenSpaceColor)[BindConst<bool>(enableScreenSpaceColor)];

		ReflectProperty(inverseProjectionMatrix)[uniformProjectionBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];

		ReflectProperty(screenSpaceColor)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(viewPosition)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(viewNormal)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(baseColor)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(depth)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(occlusion)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(metallic)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(roughness)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(shadow)[BindOutput(BindOutput::LOCAL)];
	}

	return *this;
}