// ScriptComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"

namespace PaintsNow {
	class ScriptComponent : public TAllocatedTiny<ScriptComponent, Component> {
	public:
		enum {
			SCRIPTCOMPONENT_TRANSPARENT = COMPONENT_CUSTOM_BEGIN,
			SCRIPTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		ScriptComponent(const String& name);
		~ScriptComponent() override;
		void SetHandler(IScript::Request& request, Event::EVENT_ID event, IScript::Request::Ref handler);
		Tiny::FLAG GetEntityFlagMask() const override;
		void ScriptUninitialize(IScript::Request& request) override;
		const String& GetAliasedTypeName() const override;

	protected:
		void DispatchEvent(Event& event, Entity* entity) override;
		void UpdateEntityFlagMask();

	protected:
		String name;
		Tiny::FLAG entityFlagMask;
		IScript::Request::Ref handlers[Event::EVENT_END];
	};
}

