// Annotation.h
// PaintDream (paintdream@paintdream.com)
// 2018-2-10
//

#pragma once
#include "../LostDream.h"

namespace PaintsNow {
	class Annotation : public TReflected<Annotation, LostDream::Qualifier> {
	public:
		bool Initialize() override;
		bool Run(int randomSeed, int length) override;
		void Summary() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
	};
}

