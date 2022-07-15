// ComputeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"

namespace PaintsNow {
	class ComputeComponent : public TAllocatedTiny<ComputeComponent, Component> {
	public:
		ComputeComponent();
	};
}
