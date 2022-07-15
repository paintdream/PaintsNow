// FollowComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "FollowComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class FollowComponent;
	class FollowComponentModule : public TReflected<FollowComponentModule, ModuleImpl<FollowComponent> > {
	public:
		FollowComponentModule(Engine& engine);
		~FollowComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		/// <summary>
		/// Create FollowComponent
		/// </summary>
		/// <param name="bufferSize"> the Buffer Size </param>
		/// <param name="delayInterval"> delay interval </param>
		/// <returns> the follow component </returns>
		TShared<FollowComponent> RequestNew(IScript::Request& request, uint32_t bufferSize, uint32_t delayInterval);

		/// <summary>
		/// Set source transform component
		/// </summary>
		/// <param name="followComponent"> the FollowComponent </param>
		/// <param name="transformComponent"> the source TransformComponent </param>
		/// <returns></returns>
		void RequestAttach(IScript::Request& request, IScript::Delegate<FollowComponent> followComponent, IScript::Delegate<TransformComponent> transformComponent);
	};
}

