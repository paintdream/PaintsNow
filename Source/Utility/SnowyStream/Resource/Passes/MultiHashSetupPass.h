// MultiHashSetup.h
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/MultiHashSetupFS.h"

namespace PaintsNow {
	class MultiHashSetupPass : public TReflected<MultiHashSetupPass, PassBase> {
	public:
		MultiHashSetupPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		// Vertex shaders
		StandardTransformVS standardTransform;
		// Fragment shaders
		MultiHashSetupFS shaderMultiHashSetup;
		StandardParameterFS shaderParameter;
		DeferredCompactEncodeFS shaderCompactEncode;
	};
}
