// SpaceComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-10
//

#pragma once
#include "../../Component.h"
#include "../../Entity.h"

namespace PaintsNow {
	class SpaceComponent : public TAllocatedTiny<SpaceComponent, Component> {
	public:
		SpaceComponent(bool sorted = true);
		~SpaceComponent() override;

		enum {
			SPACECOMPONENT_ORDERED = COMPONENT_CUSTOM_BEGIN,
			SPACECOMPONENT_FORWARD_EVENT_TICK = COMPONENT_CUSTOM_BEGIN << 1,
			SPACECOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 2
		};

		void Optimize();
		void QueueRoutine(Engine& engine, ITask* task);
		void QueryEntities(std::vector<TShared<Entity> >& entities, const Float3Pair& box);
		bool Insert(Engine& engine, Entity* entity);
		bool Remove(Engine& engine, Entity* entity);
		void RemoveAll(Engine& engine);
		uint32_t GetEntityCount() const;

		Entity* GetRootEntity() const;
		void SetRootEntity(Entity* entity);
		FLAG GetEntityFlagMask() const override;
		void DispatchEvent(Event& event, Entity* entity) override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) override;
		const Float3Pair& GetBoundingBox() const;
		float Raycast(RaycastTask& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const override;

	protected:
		template <class T>
		struct EntityEnumerator {
			static void ForAllEntities(Engine& engine, Entity* entity, T& op) {
				if (op(entity)) {
					if (entity->Flag().load(std::memory_order_relaxed) & Entity::ENTITY_HAS_SPACE) {
						size_t size = entity->GetComponentCount();
						for (size_t i = 0; i < size; i++) {
							Component* component = entity->GetComponent(i);
							if (component != nullptr && component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
								SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
								if ((spaceComponent->Flag().load(std::memory_order_acquire) & COMPONENT_OVERRIDE_WARP)
									&& spaceComponent->GetWarpIndex() != engine.GetKernel().GetCurrentWarpIndex()) {
									engine.GetKernel().QueueRoutine(spaceComponent, CreateTaskContextFree(ForAllEntitiesTree, std::ref(engine), TShared<Entity>(spaceComponent->GetRootEntity()), op));
								} else {
									ForAllEntitiesTree(engine, spaceComponent->GetRootEntity(), op);
								}
							}
						}
					}
				}
			}

			static void ForAllEntitiesTree(Engine& engine, const TShared<Entity>& rootEntity, T& op) {
				for (Entity* entity = rootEntity(); entity != nullptr; entity = entity->Right()) {
					if (entity->Left() != nullptr) {
						ForAllEntitiesTree(engine, entity->Left(), op);
					}

					ForAllEntities(engine, entity, op);
				}
			}
		};

	public:
		template <class T>
		static void ForAllEntities(Engine& engine, Entity* entity, T& op) {
			EntityEnumerator<T>::ForAllEntities(engine, entity, op);
		}

	protected:
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		float RoutineRaycast(RaycastTaskWarp& task, Float3Pair& ray, MatrixFloat4x4& transform, Unit* parent, float ratio) const;
		void RoutineUpdateBoundingBoxRecursive(Engine& engine, Float3Pair& box, Entity* entity, bool subEntity);
		void RoutineUpdateBoundingBox(Engine& engine, Float3Pair& box, bool subEntity);
		void RoutineDispatchEvent(const Event& event);
		void FastRemoveNode(Engine& engine, Entity* entity);
		bool ValidateEntity(Entity* entity);
		bool ValidateCycleReferences(Entity* entity);
		bool RecursiveValidateCycleReferences(Entity* entity);

	protected:
		Float3Pair boundingBox;
		Entity* rootEntity;
		uint32_t entityCount;
#ifdef _DEBUG
		Entity* hostEntity;
#endif
	};
}

