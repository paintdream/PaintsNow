// Memory.h
// PaintDream (paintdream@paintdream.com)
// 2019-10-11
//

#pragma once
#include "../LostDream.h"

namespace PaintsNow {
	class Memory : public TReflected<Memory, LostDream::Qualifier> {
	public:
		bool Initialize() override;
		bool Run(int randomSeed, int length) override;
		void Summary() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
	};
}

