#define USE_SWIZZLE
#include "MultiHashGatherFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashGatherFS::MultiHashGatherFS() {
	
}

TObject<IReflect>& MultiHashGatherFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(depthTexture);
		ReflectProperty(refDepthTexture);
		ReflectProperty(refBaseColorOcclusionTexture);
		ReflectProperty(refNormalRoughnessMetallicTexture);

		ReflectProperty(gatherParamBuffer);
		ReflectProperty(blendColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}

String MultiHashGatherFS::GetShaderText() {
	return UnifyShaderCode(
		float dummy = 0;
	);
}
