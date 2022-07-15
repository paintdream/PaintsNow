#define USE_SWIZZLE
#include "SkyTransmittanceFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyTransmittanceFS::SkyTransmittanceFS() {
	paramBuffer.description.state.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String SkyTransmittanceFS::GetShaderText() {
	return UnifyShaderCode(
		float3 retValue;
		ComputeTransmittanceToTopAtmosphereBoundaryTexture(retValue, atmosphere, rasterCoord.xy);
		transmittance.xyz = retValue;
		transmittance.w = 1;
	);
}

TObject<IReflect>& SkyTransmittanceFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(transmittance)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
