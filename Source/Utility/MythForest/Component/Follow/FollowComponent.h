// FollowComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../Transform/TransformComponent.h"

namespace PaintsNow {
	class FollowComponent : public TAllocatedTiny<FollowComponent, Component> {
	public:
		enum {
			FOLLOW_COMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN,
		};

		FollowComponent(uint32_t bufferSize, uint32_t delayInterval);
		~FollowComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void Attach(const TShared<TransformComponent>& transformComponent);
		Tiny::FLAG GetEntityFlagMask() const override;
		void DispatchEvent(Event& event, Entity* entity) override;

	protected:
		struct Record {
			Record() : timestamp(0) {}

			TransformComponent::TRSData trs;
			uint64_t timestamp;
		};

		static void Interpolate(TransformComponent::TRSData& output, const TransformComponent::TRSData& lhs, const TransformComponent::TRSData& rhs, float t);

		TShared<TransformComponent> sourceTransformComponent;
		std::vector<Record> transformBuffer;
		std::vector<TransformComponent::TRSData> computeTransformBuffer;
		uint32_t currentTransformIndex;
		uint32_t delayInterval;
	};
}

