// NavigateComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "NavigateComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class NavigateComponent;
	class NavigateComponentModule : public TReflected<NavigateComponentModule, ModuleImpl<NavigateComponent> > {
	public:
		NavigateComponentModule(Engine& engine);
		~NavigateComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create NavigateComponent
		/// </summary>
		/// <returns> NavigateComponent object </returns>
		TShared<NavigateComponent> RequestNew(IScript::Request& request);
	};
}

