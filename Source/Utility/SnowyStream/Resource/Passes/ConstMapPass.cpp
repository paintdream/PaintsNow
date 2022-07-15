#include "ConstMapPass.h"

using namespace PaintsNow;

ConstMapPass::ConstMapPass() {
	// Use uniform world
	vertexTransform.enableViewProjectionMatrix = false;
	vertexTransform.enableVertexNormal = false;
	vertexTransform.enableVertexColor = false;
	vertexTransform.enableVertexTangent = false;
	vertexTransform.enableInstancedColor = true;
	// vertexTransform.enableClampedFar = true;
}

TObject<IReflect>& ConstMapPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertexTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(constWriter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}