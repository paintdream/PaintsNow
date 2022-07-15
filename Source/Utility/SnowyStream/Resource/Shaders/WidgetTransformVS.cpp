#define USE_SWIZZLE
#include "WidgetTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

WidgetTransformVS::WidgetTransformVS() {
	vertexBuffer.description.state.usage = IRender::Resource::BufferDescription::VERTEX;
	instanceBuffer.description.state.usage = IRender::Resource::BufferDescription::INSTANCED;
	instanceExtraBuffer.description.state.usage = IRender::Resource::BufferDescription::INSTANCED;
}

String WidgetTransformVS::GetShaderText() {
	return UnifyShaderCode(
		position.xyz = vertexPosition;
		position.w = 1;
		position = mult_vec(worldMatrix, position);

		// copy outputs
		texCoord = vertexPosition.xyxy * float(0.5) + float4(0.5, 0.5, 0.5, 0.5);
		texCoord.zw = mult_vec(worldMatrix, texCoord).xy;
		texCoord = lerp(texCoordBegin, texCoordEnd, texCoord);
	);
}

TObject<IReflect>& WidgetTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertexBuffer);
		ReflectProperty(instanceBuffer);
		ReflectProperty(instanceExtraBuffer);

		ReflectProperty(vertexPosition)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(worldMatrix)[instanceBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(texCoordBegin)[instanceExtraBuffer][BindInput(BindInput::TEXCOORD)];
		ReflectProperty(texCoordEnd)[instanceExtraBuffer][BindInput(BindInput::TEXCOORD + 1)];

		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(texCoord)[BindOutput(BindOutput::TEXCOORD)];
	}

	return *this;
}