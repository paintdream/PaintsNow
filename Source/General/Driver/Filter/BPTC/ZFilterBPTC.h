// ZFilterBPTC.h
// PaintDream (paintdream@paintdream.com)
// 2019-7-29
//

#pragma once
#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterBPTC final : public IFilterBase {
	public:
		virtual IStreamBase* CreateFilter(IStreamBase& streamBase);
	};
}

