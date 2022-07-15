// ComputeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ComputeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class ComputeComponent;
	class ComputeComponentModule : public TReflected<ComputeComponentModule, ModuleImpl<ComputeComponent> > {
	public:
		ComputeComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ComputeComponent
		/// </summary>
		/// <returns> ComputeComponent object </returns>
		TShared<ComputeComponent> RequestNew(IScript::Request& request);
	};
}
