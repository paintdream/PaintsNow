#pragma once
#include "../../../../Core/Interface/IFilterBase.h"

namespace PaintsNow {
	class ZFilterLZMA final : public IFilterBase {
	public:
		IStreamBase* CreateFilter(IStreamBase& streamBase) override;
	};
}
