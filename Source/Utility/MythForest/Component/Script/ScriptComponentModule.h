// ScriptComponentModule.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "ScriptComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class ScriptComponent;
	class ScriptRoutine;
	class ScriptComponentModule : public TReflected<ScriptComponentModule, ModuleImpl<ScriptComponent> > {
	public:
		ScriptComponentModule(Engine& engine);
		~ScriptComponentModule() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		/// <summary>
		/// Create ScriptComponent
		/// </summary>
		/// <param name="name"> name </param>
		/// <returns> ScriptComponent object </returns>
		TShared<ScriptComponent> RequestNew(IScript::Request& request, const String& name);

		/// <summary>
		/// Set script handler of ScriptComponent
		/// </summary>
		/// <param name="scriptComponent"> the ScriptComponent </param>
		/// <param name="event"> event type string </param>
		/// <param name="handler"> handler callback </param>
		void RequestSetHandler(IScript::Request& request, IScript::Delegate<ScriptComponent> scriptComponent, const String& event, IScript::Request::Ref handler);

	protected:
		std::unordered_map<String, size_t> mapEventNameToID;
	};
}

