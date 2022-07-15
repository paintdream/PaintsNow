// CacheComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "CacheComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class CacheComponent;
	class CacheComponentModule : public TReflected<CacheComponentModule, ModuleImpl<CacheComponent> > {
	public:
		CacheComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<CacheComponent> RequestNew(IScript::Request& request);
		void RequestPushObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent, std::vector<IScript::Delegate<SharedTiny> >& objects);
		void RequestClearObjects(IScript::Request& request, IScript::Delegate<CacheComponent> cacheComponent);
	};
}

