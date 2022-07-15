// FieldSimplygon.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#pragma once
#include "../FieldComponent.h"

namespace PaintsNow {
	class FieldSimplygon : public FieldComponent::FieldBase {
	public:
		enum SIMPOLYGON_TYPE {
			BOUNDING_BOX, BOUNDING_SPHERE, BOUNDING_CYLINDER,
		};

		FieldSimplygon(SIMPOLYGON_TYPE type, const Float3Pair& box);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		Bytes operator [] (const Float3& position) const override final;
		void PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const override final;
		void QueryEntitiesForEntityTree(Entity* entity, std::vector<TShared<Entity> >& entities) const override final;

	protected:
		SIMPOLYGON_TYPE type;
		Float3Pair box;
	};
}

