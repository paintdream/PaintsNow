// BridgeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-16
//

#pragma once
#include "../../Component.h"

namespace PaintsNow {
	// Weak reference component
	class BridgeComponent : public TAllocatedTiny<BridgeComponent, Component> {
	public:
		BridgeComponent(const TShared<Component>& targetComponent);
		void DispatchEvent(Event& event, Entity* entity) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		Entity* GetHostEntity() const override;
		void Clear(Engine& engine);

	protected:
		Entity* hostEntity;
		TShared<Component> targetComponent;
	};
}

