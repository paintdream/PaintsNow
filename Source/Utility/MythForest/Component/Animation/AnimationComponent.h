// AnimationComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/SkeletonResource.h"

namespace PaintsNow {
	class AnimationComponent : public TAllocatedTiny<AnimationComponent, UniqueComponent<Component, SLOT_ANIMATION_COMPONENT> > {
	public:
		enum {
			ANIMATIONCOMPONENT_REPEAT = COMPONENT_CUSTOM_BEGIN,
		};

		AnimationComponent(const TShared<SkeletonResource>& skeletonResource);
		~AnimationComponent() override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void DispatchEvent(Event& event, Entity* entity) override;
		FLAG GetEntityFlagMask() const override;

		IRender::Resource* AcquireBoneMatrixBuffer(IRender& render, IRender::Queue* queue);

		void Attach(const String& name, const TShared<Entity>& entity);
		void Detach(const TShared<Entity>& entity);
		void Play(const String& clipName, float startTime);
		void RegisterEvent(const String& identifier, const String& clipName, float timeStamp);
		void SetSpeed(float speed);
		TShared<SkeletonResource> GetSkeletonResource();

	private:
		class EventData {
		public:
			String identifier;
			float timeStamp;
		};

	private:
		TShared<SkeletonResource> skeletonResource;
		std::vector<MatrixFloat4x4> boneMatrices;
		std::vector<std::pair<uint32_t, TShared<Entity> > > mountPoints;
		std::vector<std::vector<EventData> > eventData;
		IRender::Resource* boneMatrixBuffer;
		uint32_t clipIndex;
		float animationTime;
		float speed;
	};
}
