#define USE_SWIZZLE
#include "ScreenSpaceFilterFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenSpaceFilterFS::ScreenSpaceFilterFS() {
	filterBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenSpaceFilterFS::GetShaderText() {
	return UnifyShaderCode(
		rasterCoord.xy = rasterCoord.xy * invScreenSize;

		float4 coordCenter = textureLod(inputTexture, rasterCoord.xy, float(0));
		if (coordCenter.x + coordCenter.y < 0.5) {
			float4 sum = float4(0, 0, 0, 0);
			float4 values[4];
			values[0] = textureLod(inputTexture, rasterCoord.xy + float2(-invScreenSize.x, 0), float(0));
			values[1] = textureLod(inputTexture, rasterCoord.xy + float2(invScreenSize.x, 0), float(0));
			values[2] = textureLod(inputTexture, rasterCoord.xy + float2(0, -invScreenSize.y), float(0));
			values[3] = textureLod(inputTexture, rasterCoord.xy + float2(0, invScreenSize.y), float(0));

			int j = 0;
			for (int i = 0; i < 4; i++) {
				float4 v = values[i];
				if (v.x + v.y > 0.5) {
					sum = sum + v;
					j++;
				}
			}

			if (j != 0) {
				coordCenter = sum / float(j);
			}
		}

		filteredValue = coordCenter;
	);
}

TObject<IReflect>& ScreenSpaceFilterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(inputTexture);
		ReflectProperty(filterBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(invScreenSize)[filterBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(filteredValue)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
