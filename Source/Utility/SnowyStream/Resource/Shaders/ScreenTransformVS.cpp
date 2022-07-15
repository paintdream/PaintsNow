#define USE_SWIZZLE
#include "ScreenTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenTransformVS::ScreenTransformVS() : enableVertexTransform(false), enableRasterCoord(true) {
	vertexBuffer.description.state.usage = IRender::Resource::BufferDescription::VERTEX;
	transformBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenTransformVS::GetShaderText() {
	return UnifyShaderCode(
		if (enableRasterCoord) {
			rasterCoord.xy = vertexPosition.xy * float2(0.5, 0.5) + float2(0.5, 0.5);
		}

		if (enableVertexTransform) {
			position.xyz = vertexPosition.xyz;
			position.w = 1;
			position = mult_vec(worldTransform, position);
			// no far clip
			// position.z = min(position.z, position.w);
		}

		if (!enableVertexTransform) {
			position.xyz = vertexPosition.xyz;
			position.w = 1;
		}
	);
}

TObject<IReflect>& ScreenTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(enableVertexTransform)[BindConst<bool>(enableVertexTransform)];
		ReflectProperty(enableRasterCoord)[BindConst<bool>(enableRasterCoord)];

		ReflectProperty(vertexBuffer);
		ReflectProperty(transformBuffer)[BindEnable(enableVertexTransform)];

		ReflectProperty(vertexPosition)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(worldTransform)[BindEnable(enableVertexTransform)][transformBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(rasterCoord)[BindEnable(enableRasterCoord)][BindOutput(BindOutput::TEXCOORD)] ;
	}

	return *this;
}