// AnimationComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "AnimationComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class AnimationComponentModule : public TReflected<AnimationComponentModule, ModuleImpl<AnimationComponent> > {
	public:
		AnimationComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create AnimationComponent
		/// </summary>
		/// <param name="skeletonResource"> SkeletonResource of animation </param>
		/// <returns> the AnimationComponent </returns>
		TShared<AnimationComponent> RequestNew(IScript::Request& request, IScript::Delegate<SkeletonResource> skeletonResource);

		/// <summary>
		/// Play animation
		/// </summary>
		/// <param name="animationComponent"> the AnimationComponnet </param>
		/// <param name="clipName"> clip name </param>
		/// <param name="startTime"> clip time </param>
		void RequestPlay(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& clipName, float startTime);

		/// <summary>
		/// Set animation speed
		/// </summary>
		/// <param name="animationComponent"> the AnimationComponent </param>
		/// <param name="speed"> animation speed </param>
		void RequestSetSpeed(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, float speed);

		/// <summary>
		/// Attach entity to a bone
		/// </summary>
		/// <param name="animationComponent"> the AnimationComponent </param>
		/// <param name="name"> tach point bone name </param>
		/// <param name="entity"> entity to attach </param>
		void RequestAttach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& name, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Deatch entity
		/// </summary>
		/// <param name="animationComponent"> the AnimationComponent </param>
		/// <param name="entity"> entity to be detached </param>
		void RequestDetach(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Register event on animation
		/// </summary>
		/// <param name="animationComponent"> the AnimationComponent </param>
		/// <param name="identifier"> event identifier </param>
		/// <param name="clipName"> clip name </param>
		/// <param name="time"> clip time </param>
		void RequestRegisterEvent(IScript::Request& request, IScript::Delegate<AnimationComponent> animationComponent, const String& identifier, const String& clipName, float time);
	};
}

