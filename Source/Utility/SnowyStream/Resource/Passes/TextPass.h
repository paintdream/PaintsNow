// TextPass.h
// PaintDream (paintdream@paintdream.com)
// 2021-1-17
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/TextTransformVS.h"
#include "../Shaders/TextShadingFS.h"

namespace PaintsNow {
	class TextPass : public TReflected<TextPass, PassBase> {
	public:
		TextPass();

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TextTransformVS textTransform;
		TextShadingFS textShading;
	};
}
