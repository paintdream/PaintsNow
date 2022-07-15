#define USE_SWIZZLE
#include "ScreenSpaceTraceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenSpaceTraceFS::ScreenSpaceTraceFS() {
	traceBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenSpaceTraceFS::GetShaderText() {
	return UnifyShaderCode(
		rasterCoord.xy = rasterCoord.xy * invScreenSize.xy;
		float depth = textureLod(depthTexture, rasterCoord.xy, float(0)).x;
		float4 position;
		position.xyz = unprojection(inverseProjectionParams, float3(rasterCoord.x, rasterCoord.y, depth) * float(2) - float3(1, 1, 1));

		float3 viewNormal;
		viewNormal.xy = textureLod(normalTexture, rasterCoord.xy, float(0)).xy * float(255.0 / 127.0) - float2(128.0 / 127.0, 128.0 / 127.0);
		viewNormal.z = sqrt(max(0.0, 1 - dot(viewNormal.xy, viewNormal.xy)));

		float3 direction = projection(inverseProjectionParams, reflect(normalize(position.xyz - float3(0, 0, 1)), viewNormal)).xyw;

		position = projection(projectionParams, position.xyz);
		float3 startPos = position.xyw;
		startPos.xy = startPos.xy * float(0.5) + startPos.zz * float(0.5);
		float3 stepPos = direction * invScreenSize.x;
		stepPos.xy = stepPos.xy * float(0.5) + stepPos.zz * float(0.5);

		traceCoord = float4(0, 0, 0, 0);
		float lastDepth = startPos.z;
		float t = 0.1;
		for (int i = 0; i < 16; i++) {
			float3 pos = startPos + stepPos * t;

			// convert to screen space coord
			// float d = textureLod(depthTexture, pos.xy / pos.z * float(0.5) + float2(0.5, 0.5), float(0)).x * float(2.0) - float(1.0);
			float2 coord = pos.xy / pos.z;
			float d = textureLod(depthTexture, coord, float(0)).x;
			if (step(lastDepth, d) * step(d, pos.z) > 0) {
				traceCoord.xy = coord;
				// traceCoord = textureLod(normalTexture, traceCoord.xy, float(0));
				break;
			}

			startPos = pos;
			t = t * float(2);
			lastDepth = d;
		}

		//traceCoord.xy = float2(direction.z, 0.0);
	);
}

TObject<IReflect>& ScreenSpaceTraceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(depthTexture);
		ReflectProperty(normalTexture);
		ReflectProperty(traceBuffer);

		ReflectProperty(projectionParams)[traceBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(inverseProjectionParams)[traceBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[traceBuffer][BindInput(BindInput::GENERAL)];

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(traceCoord)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
