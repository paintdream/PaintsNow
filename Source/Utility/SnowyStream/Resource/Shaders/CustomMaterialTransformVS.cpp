#include "CustomMaterialTransformVS.h"

using namespace PaintsNow;

CustomMaterialTransformVS::CustomMaterialTransformVS() {
	description = TShared<CustomMaterialDescription>::From(new CustomMaterialDescription());
}

String CustomMaterialTransformVS::GetShaderText() {
	return description->code;
}

TObject<IReflect>& CustomMaterialTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		description->ReflectExternal(reflect, instanceData);
	}

	return *this;
}
