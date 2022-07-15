// TerrainComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "TerrainComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class TerrainComponent;
	class TerrainComponentModule : public TReflected<TerrainComponentModule, ModuleImpl<TerrainComponent> > {
	public:
		TerrainComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create TerrainComponent
		/// </summary>
		/// <param name="terrainResource"> TerrainResource object </param>
		/// <returns> TerrainComponent object </returns>
		TShared<TerrainComponent> RequestNew(IScript::Request& request, IScript::Delegate<TerrainResource> terrainResource);
	};
}
