// ZFilterPod.h
// PaintDream (paintdream@paintdream.com)
// 2015-6-10
//

#pragma once
#include "../../../Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterPod final : public IFilterBase {
	public:
		ZFilterPod();
		~ZFilterPod() override;
		IStreamBase* CreateFilter(IStreamBase& stream) override;
	};
}

