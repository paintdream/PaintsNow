// MultiHashTrace.h
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashTraceFS.h"

namespace PaintsNow {
	class MultiHashTracePass : public TReflected<MultiHashTracePass, PassBase> {
	public:
		MultiHashTracePass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		ScreenTransformVS screenTransform;
		MultiHashTraceFS shaderMultiHashTrace;
	};
}
