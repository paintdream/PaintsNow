#include "IShader.h"
using namespace PaintsNow;

IShader::MetaShader::MetaShader(IRender::Resource::ShaderDescription::Stage v) : shaderType(v) {}

IShader::MetaShader IShader::MetaShader::operator = (IRender::Resource::ShaderDescription::Stage value) {
	return MetaShader(value);
}

TObject<IReflect>& IShader::MetaShader::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(shaderType);
	}

	return *this;
}

String IShader::GetShaderText() {
	return "";
}

String IShader::GetPredefines() {
	return "";
}

UInt3 IShader::WorkGroupSize;
UInt3 IShader::NumWorkGroups;
UInt3 IShader::LocalInvocationID;
UInt3 IShader::WorkGroupID;
UInt3 IShader::GlobalInvocationID;
uint32_t IShader::LocalInvocationIndex;
