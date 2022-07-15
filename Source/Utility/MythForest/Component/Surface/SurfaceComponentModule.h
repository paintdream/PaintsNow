// SurfaceComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "SurfaceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class SurfaceComponent;
	class SurfaceComponentModule : public TReflected<SurfaceComponentModule, ModuleImpl<SurfaceComponent> > {
	public:
		SurfaceComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create SurfaceComponent
		/// </summary>
		/// <returns> SurfaceComponent object </returns>
		TShared<SurfaceComponent> RequestNew(IScript::Request& request);
	};
}
