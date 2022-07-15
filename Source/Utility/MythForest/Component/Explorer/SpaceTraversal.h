// SpaceTraversal.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-25
//

#pragma once
#include "../../../../Core/Template/TProxy.h"
#include "../Renderable/RenderableComponent.h"
#include "../Space/SpaceComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

namespace PaintsNow {
	template <class T, class Config>
	class SpaceTraversal {
	public:
		typedef typename Config::TaskData TaskData;
		typedef typename Config::WorldInstanceData WorldInstanceData;
		typedef typename Config::CaptureData CaptureData;

		template <class D>
		void CollectComponentsFromEntityTree(Engine& engine, TaskData& taskData, WorldInstanceData& instanceData, const CaptureData& captureData, Entity* rootEntity, D ordered) {
			for (Entity* entity = rootEntity; entity != nullptr; entity = entity->Right()) {
				if (!getboolean<D>::value && !captureData(instanceData.boundingBox))
					break;

				if (!taskData.Continue()) {
					return;
				}

				Entity* leftEntity = entity->Left();
				if (leftEntity != nullptr) {
					IMemory::PrefetchRead(reinterpret_cast<uint8_t*>(leftEntity));
					if /*constexpr*/ (sizeof(Entity) > CPU_CACHELINE_SIZE) {
						IMemory::PrefetchRead(reinterpret_cast<uint8_t*>(leftEntity) + CPU_CACHELINE_SIZE);
					}
				}

				Entity* rightEntity = entity->Right();
				if (rightEntity != nullptr) {
					IMemory::PrefetchRead(reinterpret_cast<uint8_t*>(rightEntity));
					if /*constexpr*/ (sizeof(Entity) > CPU_CACHELINE_SIZE) {
						IMemory::PrefetchRead(reinterpret_cast<uint8_t*>(rightEntity) + CPU_CACHELINE_SIZE);
					}
				}

				size_t size = entity->GetComponentCount();
				if (size != 0) {
					IMemory::PrefetchReadLocal(entity->GetComponent(0));

					Tiny::FLAG flag = entity->Flag().load(std::memory_order_relaxed);
					if ((flag & Tiny::TINY_ACTIVATED) && captureData(entity->GetKey())) {
						(static_cast<T*>(this))->CollectComponents(engine, taskData, instanceData, captureData, entity);
					}

					if (!taskData.Continue()) {
						return;
					}
				}

				// Cull left & right 
				if (leftEntity != nullptr) {
					if /*constexpr*/ (getboolean<D>::value) {
						uint8_t index = entity->GetIndex();
						float save = Entity::Meta::SplitPush(std::true_type(), instanceData.boundingBox, entity->GetKey(), index);

						// cull right
						CollectComponentsFromEntityTree(engine, taskData, instanceData, captureData, leftEntity, ordered);

						Entity::Meta::SplitPop(instanceData.boundingBox, index, save);
					} else {
						CollectComponentsFromEntityTree(engine, taskData, instanceData, captureData, leftEntity, ordered);
					}
				}
			}
		}

		void CollectComponentsFromSpace(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, SpaceComponent* spaceComponent) {
			OPTICK_EVENT();
			std::atomic<uint32_t>& pendingCount = reinterpret_cast<std::atomic<uint32_t>&>(taskData.pendingCount);

			assert(pendingCount.load(std::memory_order_acquire) != 0); // must increase it before calling me.
			if (!taskData.Continue()) {
				if (pendingCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
					(static_cast<T*>(this))->CompleteCollect(engine, taskData);
				}
			} else {
				if ((spaceComponent->Flag().load(std::memory_order_relaxed) & Component::COMPONENT_OVERRIDE_WARP) && spaceComponent->GetWarpIndex() != engine.GetKernel().GetCurrentWarpIndex()) {
					spaceComponent->QueueRoutine(engine, CreateTaskContextFree(Wrap(this, &SpaceTraversal<T, Config>::CollectComponentsFromSpace), std::ref(engine), std::ref(taskData), instanceData, captureData, spaceComponent));
				} else {
					Entity* spaceRoot = spaceComponent->GetRootEntity();
					if (spaceRoot != nullptr) {
						// Make new world instance data
						WorldInstanceData subWorldInstancedData = instanceData;
						// update bounding box
						subWorldInstancedData.boundingBox = spaceComponent->GetBoundingBox();
						if (spaceComponent->Flag().load(std::memory_order_relaxed) & SpaceComponent::SPACECOMPONENT_ORDERED) {
							CollectComponentsFromEntityTree(engine, taskData, subWorldInstancedData, captureData, spaceRoot, std::true_type());
						} else {
							CollectComponentsFromEntityTree(engine, taskData, subWorldInstancedData, captureData, spaceRoot, std::false_type());
						}
					}

					if (pendingCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
						(static_cast<T*>(this))->CompleteCollect(engine, taskData);
					}
				}
			}
		}

		void CollectComponentsFromEntity(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
			size_t size = entity->GetComponentCount();
			std::atomic<uint32_t>& pendingCount = reinterpret_cast<std::atomic<uint32_t>&>(taskData.pendingCount);

			pendingCount.fetch_add(1, std::memory_order_relaxed);
			for (size_t i = 0; i < size; i++) {
				Component* component = entity->GetComponent(i);
				if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
					SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
					pendingCount.fetch_add(1, std::memory_order_relaxed);
					CollectComponentsFromSpace(engine, taskData, instanceData, captureData, spaceComponent);
				}
			}

			if (pendingCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
				(static_cast<T*>(this))->CompleteCollect(engine, taskData);
			}
		}
	};
}

