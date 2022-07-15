// Component.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "../../Core/Template/TAllocator.h"
#include "../../Core/Template/TCache.h"
#include "Unit.h"
#include "Event.h"

namespace PaintsNow {
	class ResourceManager;
	class Entity;
	class Component : public TReflected<Component, Unit> {
	public:
		typedef Component BaseComponent;
		enum {
			COMPONENT_OVERRIDE_WARP = UNIT_CUSTOM_BEGIN,
			COMPONENT_SHARED = UNIT_CUSTOM_BEGIN << 1,
			COMPONENT_ALIASED_TYPE = UNIT_CUSTOM_BEGIN << 2,
			COMPONENT_CUSTOM_BEGIN = UNIT_CUSTOM_BEGIN << 3,
		};

		class RaycastResult {
		public:
			MatrixFloat4x4 transform;
			Float3 position; // local position
			float squareDistance;
			Float4 coord;
			uint32_t faceIndex;
			TShared<Unit> unit;
			TShared<Unit> parent;
			Unique metaType;
			Bytes metaData;
		};

		class RaycastTask : public TReflected<RaycastTask, WarpTiny> {
		public:
			RaycastTask(BytesCache* cacheOptional = nullptr);
			enum {
				RAYCASTTASK_IGNORE_WARP = TINY_CUSTOM_BEGIN,
				RAYCASTTASK_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 1
			};

			virtual bool EmplaceResult(rvalue<RaycastResult> item) = 0;

			BytesCache* cache;
			float clipOffDistance;
		};

		class RaycastTaskSerial : public TReflected<RaycastTaskSerial, RaycastTask> {
		public:
			RaycastTaskSerial(BytesCache* cacheOptional = nullptr);
			bool EmplaceResult(rvalue<RaycastResult> item) override;

			RaycastResult result;
		};

		class RaycastTaskWarp : public TReflected<RaycastTaskWarp, RaycastTask> {
		public:
			RaycastTaskWarp(Engine& engine, uint32_t maxCount);
			~RaycastTaskWarp() override;

			virtual void Finish(rvalue<std::vector<RaycastResult> > finalResult) = 0;
			bool EmplaceResult(rvalue<RaycastResult> item) override;
			bool EmplaceResult(std::vector<RaycastResult>& result, rvalue<RaycastResult> item);
			void AddPendingTask();
			void RemovePendingTask();
			Engine& GetEngine();

		protected:
			Engine& engine;
			uint32_t maxCount;
			std::atomic<uint32_t> pendingCount;
			std::vector<std::vector<RaycastResult> > results;
		};

		virtual void Initialize(Engine& engine, Entity* entity);
		virtual void Uninitialize(Engine& engine, Entity* entity);
		virtual void DispatchEvent(Event& event, Entity* entity);
		virtual Entity* GetHostEntity() const;
		virtual const String& GetAliasedTypeName() const;
		virtual void UpdateBoundingBox(Engine& engine, Float3Pair& boundingBox, bool recursive);
		virtual float Raycast(RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio = 1) const;
		static void RaycastForEntity(RaycastTask& task, const Float3Pair& quickRay, Float3Pair& ray, MatrixFloat4x4& transform, Entity* entity, float ratio);
		virtual FLAG GetEntityFlagMask() const;
		virtual uint32_t GetQuickUniqueID() const;
		static inline uint32_t StaticGetQuickUniqueID() { return ~(uint32_t)0; }
	};

	enum UniqueQuickID {
		SLOT_TRANSFORM_COMPONENT = 0,
		SLOT_EXPLORER_COMPONENT,
		SLOT_ANIMATION_COMPONENT,
	};

	template <class T, uint32_t quickID = ~(uint32_t)0>
	class UniqueComponent : public TReflected<UniqueComponent<T, quickID>, T> {
	public:
		UniqueComponent() { T::Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed); }
		static inline uint32_t StaticGetQuickUniqueID() { return quickID; }

	private:
		uint32_t GetQuickUniqueID() const final { return StaticGetQuickUniqueID(); }
	};
}

