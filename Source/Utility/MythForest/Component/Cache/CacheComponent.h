// CacheComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../../Core/Interface/IType.h"

namespace PaintsNow {
	class CacheComponent : public TAllocatedTiny<CacheComponent, Component> {
	public:
		CacheComponent();
		void PushObjects(rvalue<std::vector<TShared<SharedTiny> > > objects);
		void ClearObjects();

	protected:
		std::vector<TShared<SharedTiny> > cachedObjects;
	};
}

