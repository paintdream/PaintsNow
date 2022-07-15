// NavigateComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	class NavigateComponent : public TAllocatedTiny<NavigateComponent, Component> {
	public:
		NavigateComponent();
		~NavigateComponent() override;
	};
}

