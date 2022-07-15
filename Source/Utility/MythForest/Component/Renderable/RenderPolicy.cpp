#include "RenderPolicy.h"

using namespace PaintsNow;

RenderPolicy::RenderPolicy() : priorityRange(0, 1), sortType(SORT_NONE) {}

TObject<IReflect>& RenderPolicy::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderPortName);
		ReflectProperty(priorityRange);
	}

	return *this;
}