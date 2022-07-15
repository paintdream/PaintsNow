// Allocator.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-27
//

#pragma once
#include "../LostDream.h"

namespace PaintsNow {
	class TaskAllocator : public TReflected<TaskAllocator, LostDream::Qualifier> {
	public:
		bool Initialize() override;
		bool Run(int randomSeed, int length) override;
		void Summary() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
	};
}