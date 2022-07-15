// AnalyticCurveResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class AnalyticCurveResource : public TReflected<AnalyticCurveResource, RenderResourceBase> {
	public:
		AnalyticCurveResource(ResourceManager& manager, const String& uniqueID);
	};
}

