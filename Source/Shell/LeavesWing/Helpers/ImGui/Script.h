// Script.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#pragma once
#include "IWidget.h"

namespace PaintsNow {
	class Script : public IWidget {
	public:
		void TickRender(LeavesFlute& leavesFlute) override;
	};
}

