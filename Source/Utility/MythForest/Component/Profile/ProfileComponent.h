// ProfileComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"

namespace PaintsNow {
	class ProfileComponent : public TAllocatedTiny<ProfileComponent, Component> {
	public:
		ProfileComponent(float historyRatio);
		Tiny::FLAG GetEntityFlagMask() const final;
		void DispatchEvent(Event& event, Entity* entity) final;
		float GetTickInterval() const;

	protected:
		int64_t timeStamp;
		float tickInterval;
		float historyRatio;
	};
}
