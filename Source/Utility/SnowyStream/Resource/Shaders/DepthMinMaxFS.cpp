#define USE_SWIZZLE
#include "DepthMinMaxFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

DepthMinMaxFS::DepthMinMaxFS() {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String DepthMinMaxFS::GetShaderText() {
	return UnifyShaderCode(
		float2 depth0 = texture(depthTexture, rasterCoord.xy).xy;
		float2 depth1 = texture(depthTexture, rasterCoord.xy + float2(invScreenSize.x, 0)).xy;
		float2 depth2 = texture(depthTexture, rasterCoord.xy + float2(0, invScreenSize.y)).xy;
		float2 depth3 = texture(depthTexture, rasterCoord.xy + float2(invScreenSize.x, invScreenSize.y)).xy;
		
		outputDepth.x = min(min(depth0.x, depth1.x), min(depth2.x, depth3.x));
		outputDepth.y = max(max(depth0.y, depth1.y), max(depth2.y, depth3.y));
	);
}

TObject<IReflect>& DepthMinMaxFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(invScreenSize)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(outputDepth)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
