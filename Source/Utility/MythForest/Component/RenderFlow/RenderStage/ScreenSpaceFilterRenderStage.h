// ScreenSpaceFilterRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/ScreenSpaceFilterPass.h"

namespace PaintsNow {
	class ScreenSpaceFilterRenderStage : public TReflected<ScreenSpaceFilterRenderStage, GeneralRenderStageDraw<ScreenSpaceFilterPass> > {
	public:
		ScreenSpaceFilterRenderStage(const String& s);

		RenderPortTextureInput Depth;
	};
}

