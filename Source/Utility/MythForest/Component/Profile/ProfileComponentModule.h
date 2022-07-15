// ProfileComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "ProfileComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class ProfileComponent;
	class ProfileComponentModule : public TReflected<ProfileComponentModule, ModuleImpl<ProfileComponent> > {
	public:
		ProfileComponentModule(Engine& engine);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ProfileComponent
		/// </summary>
		/// <param name="historyRatio"> history interpolation ratio </param>
		/// <returns> ProfileComponent object </returns>
		TShared<ProfileComponent> RequestNew(IScript::Request& request, float historyRatio);

		/// <summary>
		/// Get ProfileComponent tick interval
		/// </summary>
		/// <param name="profileComponent"> the ProfileComponent </param>
		/// <returns> tick interval </returns>
		float RequestGetInterval(IScript::Request& request, IScript::Delegate<ProfileComponent> profileComponent);
	};
}
