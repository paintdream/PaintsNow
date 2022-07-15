// PhaseComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "PhaseComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class PhaseComponent;
	class PhaseComponentModule : public TReflected<PhaseComponentModule, ModuleImpl<PhaseComponent> > {
	public:
		PhaseComponentModule(Engine& engine);
		~PhaseComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create PhaseComponent
		/// </summary>
		/// <param name="renderFlowComponent"> the RenderFlowComponent that PhaseComponent binds to </param>
		/// <param name="portName"> the port name that PhaseComponent read/write </param>
		/// <returns> PhaseComponent object </returns>
		TShared<PhaseComponent> RequestNew(IScript::Request& request, IScript::Delegate<RenderFlowComponent> renderFlowComponent, const String& portName);

		/// <summary>
		/// Setup parameters for PhaseComponent
		/// </summary>
		/// <param name="phaseComponent"> the PhaseComponent </param>
		/// <param name="phaseCount"> phase count </param>
		/// <param name="taskCount"> task count per batch </param>
		/// <param name="range"> effect range </param>
		/// <param name="resolution"> phase texture resolution </param>
		void RequestSetup(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount, uint32_t taskCount, const Float3& range, const UShort2& resolution);

		/// <summary>
		/// Set new phase focus center and update phase data
		/// </summary>
		/// <param name="phaseComponent"> the PhaseComponent </param>
		/// <param name="center"> phase center </param>
		void RequestUpdate(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const Float3& center);

		/// <summary>
		/// Step phase baking
		/// </summary>
		/// <param name="phaseComponent"> the PhaseComponent </param>
		/// <param name="bounceCount"> bounce count of light </param>
		void RequestStep(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t bounceCount);

		/// <summary>
		/// Resample PhaseComponent with new phase count.
		/// </summary>
		/// <param name="phaseComponent"> the PhaseComponent </param>
		/// <param name="phaseCount"> phase count </param>
		void RequestResample(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, uint32_t phaseCount);

		/// <summary>
		/// Set PhaseComponent baking root entity
		/// </summary>
		/// <param name="request"></param>
		/// <param name="phaseComponent"></param>
		/// <param name="rootEntity"></param>
		void RequestBindRootEntity(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, IScript::Delegate<Entity> rootEntity);

		/// <summary>
		/// Enable debug dump mode for PhaseComponent
		/// </summary>
		/// <param name="phaseComponent"> the PhaseComponent </param>
		/// <param name="debugPath"> output dump file path </param>
		void RequestSetDebugMode(IScript::Request& request, IScript::Delegate<PhaseComponent> phaseComponent, const String& debugPath);
	};
}

