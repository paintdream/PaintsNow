// BridgeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#pragma once
#include "BridgeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class BridgeComponent;
	class BridgeComponentModule : public TReflected<BridgeComponentModule, ModuleImpl<BridgeComponent> > {
	public:
		BridgeComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<BridgeComponent> New(const TShared<Component>& component);
	
		/// <summary>
		/// Create bridge component
		/// </summary>
		/// <param name="component"> the inner component to keep </param>
		/// <returns></returns>
		TShared<BridgeComponent> RequestNew(IScript::Request& request, IScript::Delegate<Component> component);
	};
}

