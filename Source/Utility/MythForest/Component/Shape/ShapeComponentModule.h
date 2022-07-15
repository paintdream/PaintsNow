// ShapeComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ShapeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ShapeComponent;
	class ShapeComponentModule : public TReflected<ShapeComponentModule, ModuleImpl<ShapeComponent> > {
	public:
		ShapeComponentModule(Engine& engine);
		~ShapeComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ShapeComponent
		/// </summary>
		/// <param name="meshResource"> MeshResource object </param>
		/// <returns> ShapeComponent object </returns>
		TShared<ShapeComponent> RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource);
	};
}

