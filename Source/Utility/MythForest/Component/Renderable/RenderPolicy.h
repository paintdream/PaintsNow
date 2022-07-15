// RenderPolicy.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#pragma once
#include "../../../../Core/System/Tiny.h"
#include "../../../../General/Interface/IRender.h"

namespace PaintsNow {
	class RenderPolicy : public TReflected<RenderPolicy, SharedTiny> {
	public:
		RenderPolicy();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		enum SortType {
			SORT_NONE = 0,
			SORT_NEAR_TO_FAR = 1 << 0,
			SORT_FAR_TO_NEAR = 1 << 1,
			SORT_MATERIAL = 1 << 2,
			SORT_RENDERSTATE = 1 << 3, 
		};

		String renderPortName;
		uint32_t sortType;
		std::pair<uint16_t, uint16_t> priorityRange;
		IRender::Resource::RenderStateDescription renderStateTemplate;
		IRender::Resource::RenderStateDescription renderStateMask;
	};
}

