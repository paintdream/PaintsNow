// ConstMapPass.h
// PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/ConstMapFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ConstMapPass : public TReflected<ConstMapPass, PassBase> {
	public:
		ConstMapPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		// Vertex shaders
		StandardTransformVS vertexTransform;
		// Fragment shaders
		ConstMapFS constWriter;
	};
}
