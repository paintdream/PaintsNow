// ZFilterLAME.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-13
//

#pragma once
#include "../../../Interface/IAudio.h"

namespace PaintsNow {
	class ZFilterLAME final : public IFilterBase {
	public:
		IStreamBase* CreateFilter(IStreamBase& inputStream) override;
	};
}

