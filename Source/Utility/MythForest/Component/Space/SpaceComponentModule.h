// SpaceComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-10
//

#pragma once
#include "SpaceComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class SpaceComponent;
	class SpaceComponentModule : public TReflected<SpaceComponentModule, ModuleImpl<SpaceComponent> > {
	public:
		SpaceComponentModule(Engine& engine);

		/// <summary>
		/// Create SpaceComponent
		/// </summary>
		/// <param name="warpIndex"> warp index of SpaceComponent </param>
		/// <param name="sorted"> is sub entities sorted </param>
		/// <returns> SpaceComponent object </returns>
		TShared<SpaceComponent> RequestNew(IScript::Request& request, int32_t warpIndex, bool sorted);

		/// <summary>
		/// Set event forward mask, masked event will be through SpaceComponent, to sub entities.
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		/// <param name="forwardMask"> forward mask </param>
		void RequestSetForwardMask(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, uint32_t forwardMask);

		/// <summary>
		/// Insert entity to SpaceComponent
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		/// <param name="entity"> the entity to insert </param>
		void RequestInsertEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Remote entituy from SpaceComponent
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		/// <param name="entity"> the entity to remove </param>
		void RequestRemoveEntity(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, IScript::Delegate<Entity> entity);

		/// <summary>
		/// Query entities in box 
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		/// <param name="box"> query box </param>
		/// <returns> result list with { Entity } </returns>
		void RequestQueryEntities(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent, const Float3Pair& box);

		/// <summary>
		/// Get entity count of SpaceComponent
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		/// <returns> enitty count </returns>
		uint32_t RequestGetEntityCount(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent);

		/// <summary>
		/// Optimize the tree (only for ordered space components)
		/// </summary>
		/// <param name="spaceComponent"> the SpaceComponent </param>
		void RequestOptimize(IScript::Request& request, IScript::Delegate<SpaceComponent> spaceComponent);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void ScriptUninitialize(IScript::Request& request) override;
	};
}

