// ParticleComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ParticleComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ParticleComponent;
	class ParticleComponentModule : public TReflected<ParticleComponentModule, ModuleImpl<ParticleComponent> > {
	public:
		ParticleComponentModule(Engine& engine);
		~ParticleComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ParticleComponent
		/// </summary>
		/// <returns> ParticleComponent object </returns>
		TShared<ParticleComponent> RequestNew(IScript::Request& request);
	};
}

