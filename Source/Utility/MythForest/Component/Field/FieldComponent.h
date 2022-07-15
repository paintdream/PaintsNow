// FieldComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	class FieldComponent : public TAllocatedTiny<FieldComponent, Component> {
	public:
		FieldComponent();
		~FieldComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		Bytes operator [] (const Float3& position) const;

		class pure_interface FieldBase : public TReflected<FieldBase, SharedTiny> {
		public:
			virtual Bytes operator [] (const Float3& position) const = 0;
			virtual void PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const;
			virtual void QueryEntitiesForEntityTree(Entity* entity, std::vector<TShared<Entity> >& entities) const;
		};

		void SetField(const TShared<FieldBase>& field);
		void PostEvent(SpaceComponent* spaceComponent, Event& event, FLAG mask) const;
		void QueryEntities(SpaceComponent* spaceComponent, std::vector<TShared<Entity> >& entities) const;

	protected:
		uint32_t subType;
		TShared<FieldBase> fieldImpl;
	};
}

