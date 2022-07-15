// LayoutComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	class SpaceComponent;
	// Auto-arranged space management
	class LayoutComponent : public TAllocatedTiny<LayoutComponent, UniqueComponent<Component> > {
	public:
		enum {
			LAYOUT_ADAPTABLE = COMPONENT_CUSTOM_BEGIN << 0,
			LAYOUT_VERTICAL = COMPONENT_CUSTOM_BEGIN << 1,
			LAYOUT_REQUIRE_BATCH = COMPONENT_CUSTOM_BEGIN << 2,
			LAYOUT_BATCHING = COMPONENT_CUSTOM_BEGIN << 3,
			LAYOUT_BATCHED = COMPONENT_CUSTOM_BEGIN << 4,
			LAYOUT_DISPLAY = COMPONENT_CUSTOM_BEGIN << 5,
			LAYOUT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 6
		};

		LayoutComponent();
		~LayoutComponent() override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		Entity* GetHostEntity() const override;
		void DoLayout(Engine& engine, const MatrixFloat4x4& transform);
		void SetUpdateMark();

	protected:
		void UpdateTransform(const MatrixFloat4x4& transform);
		void RoutineLayoutForSpaceComponent(Engine& engine, const TShared<SpaceComponent>& spaceComponent, const MatrixFloat4x4& transform);
		Entity* hostEntity;

	public:
		Float2 scrollSize;
		Float2 scrollOffset;
		Float2Pair padding;
		Float2Pair margin;
		Float2Pair size;
		Float2Pair rect;
		int32_t weight;
		int32_t count;
		uint32_t start;
		float remained;

		Float4 clippedRect;
		Float4 percentage;

		std::atomic<int32_t> runningLayoutCount;
	};
}

