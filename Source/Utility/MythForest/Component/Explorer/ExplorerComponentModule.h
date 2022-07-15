// ExplorerComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ExplorerComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ExplorerComponent;
	class ExplorerComponentModule : public TReflected<ExplorerComponentModule, ModuleImpl<ExplorerComponent> > {
	public:
		ExplorerComponentModule(Engine& engine);
		~ExplorerComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ExplorerComponent
		/// </summary>
		/// <param name="identifier"> explorer type </param>
		/// <returns> ExplorerComponent object </returns>
		TShared<ExplorerComponent> RequestNew(IScript::Request& request, const String& identifier);

		/// <summary>
		/// Set proxy config for ExplorerComponent
		/// </summary>
		/// <param name="explorerComponent"> the ExplorerComponent </param>
		/// <param name="component"> target component </param>
		/// <param name="activateThreshold"> target component activate threshold </param>
		/// <param name="deactivateThreshold"> target component deactive threshold </param>
		void RequestSetProxyConfig(IScript::Request& request, IScript::Delegate<ExplorerComponent> explorerComponent, IScript::Delegate<Component> component, float activateThreshold, float deactivateThreshold);
	};
}

