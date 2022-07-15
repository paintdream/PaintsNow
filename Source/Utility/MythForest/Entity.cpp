#include "Entity.h"
#include "Component.h"
#include "Engine.h"
#include "../../Core/Interface/IMemory.h"

using namespace PaintsNow;

Entity::Entity(Engine& engine) {
	Flag().fetch_or(Tiny::TINY_ACTIVATED, std::memory_order_relaxed);

	SetEngineInternal(engine);
	engine.NotifyUnitConstruct(this);
	key = Float3Pair(Float3(0, 0, 0), Float3(0, 0, 0));

	assert(QueryInterface(UniqueType<WarpTiny>()) != nullptr);
}

Entity::~Entity() {
	assert(components.IsEmpty());
	Engine& engine = GetEngineInternal();
	engine.NotifyUnitDestruct(this);
}

static void InvokeClearComponentsAndRelease(void* request, bool run, Engine& engine, Entity* entity) {
	uint32_t orgIndex = engine.GetKernel().GetCurrentWarpIndex();
	uint32_t warpIndex = entity->GetWarpIndex();

	if (!run) {
		if (orgIndex != warpIndex) {
			bool result = engine.GetKernel().WaitWarp(warpIndex);
			assert(result);
		}
	} else {
		assert(engine.GetKernel().GetCurrentWarpIndex() == entity->GetWarpIndex());
	}

	entity->ClearComponents(engine);
	entity->ReleaseObject();

	if (!run) {
		if (orgIndex != ~(uint32_t)0 && orgIndex != warpIndex) {
			engine.GetKernel().WaitWarp(orgIndex);
		}
	}
}

void Entity::Destroy() {
	if (Flag().load(std::memory_order_acquire) & ENTITY_STORE_ENGINE) {
		if (!components.IsEmpty()) {
			Engine& engine = GetEngineInternal();
			Kernel& kernel = engine.GetKernel();

			if (kernel.GetCurrentWarpIndex() == GetWarpIndex()) {
				ClearComponents(engine);
			} else {
				kernel.QueueRoutine(this, CreateTask(Wrap(InvokeClearComponentsAndRelease), std::ref(engine), this));
				return;
			}
		}
	}

	assert(components.IsEmpty());
	BaseClass::Destroy();
}

void Entity::SetEngineInternal(Engine& engine) {
	// reuse parent node as engine pointer
	assert(!(Flag().load(std::memory_order_acquire) & ENTITY_STORE_ENGINE));
	Flag().fetch_or(ENTITY_STORE_ENGINE, std::memory_order_relaxed);

	reinterpret_cast<Engine*&>(*(void**)&_parentNode) = &engine;
}

void Entity::CleanupEngineInternal() {
	Flag().fetch_and(~ENTITY_STORE_ENGINE, std::memory_order_relaxed);

	reinterpret_cast<Engine*&>(*(void**)&_parentNode) = nullptr;
}

Engine& Entity::GetEngineInternal() const {
	assert(GetParent() != nullptr);
	assert(Flag().load(std::memory_order_acquire) & ENTITY_STORE_ENGINE);

	return *reinterpret_cast<Engine*>(*(void**)&_parentNode);
}

void Entity::InitializeComponent(Engine& engine, Component* component) {
	component->ReferenceObject();
	component->Initialize(engine, this);

	// add component mask
	Flag().fetch_or(component->GetEntityFlagMask(), std::memory_order_relaxed);

	if (Flag().load(std::memory_order_acquire) & ENTITY_HAS_TACH_EVENT) {
		Event event(engine, Event::EVENT_ATTACH_COMPONENT, this, component);
		PostEvent(event, ENTITY_HAS_TACH_EVENT);
	}

	if ((Flag().load(std::memory_order_acquire) & (TINY_ACTIVATED | ENTITY_HAS_ACTIVE_EVENT)) == (TINY_ACTIVATED | ENTITY_HAS_ACTIVE_EVENT)) {
		Event eventActivate(engine, Event::EVENT_ENTITY_ACTIVATE, this, nullptr);
		component->DispatchEvent(eventActivate, this);
	}
}

void Entity::UninitializeComponent(Engine& engine, Component* component) {
	if (Flag().load(std::memory_order_acquire) & ENTITY_HAS_TACH_EVENT) {
		Event event(engine, Event::EVENT_DETACH_COMPONENT, this, component);
		PostEvent(event, ENTITY_HAS_TACH_EVENT);
	}

	if ((Flag().load(std::memory_order_relaxed) | component->GetEntityFlagMask()) & ENTITY_HAS_ACTIVE_EVENT) {
		Event eventDeactivate(engine, Event::EVENT_ENTITY_DEACTIVATE, this, nullptr);
		component->DispatchEvent(eventDeactivate, this);
	}

	component->Uninitialize(engine, this);
	component->ReleaseObject();
}

void Entity::AddComponent(Engine& engine, Component* component) {
	assert((component->Flag().load(std::memory_order_acquire) & Component::COMPONENT_OVERRIDE_WARP) || component->GetWarpIndex() == GetWarpIndex());
	uint32_t id = component->GetQuickUniqueID();
	if (id != ~(uint32_t)0) {
		assert(component->Flag().load(std::memory_order_relaxed) & TINY_UNIQUE);
		size_t size = components.GetCount();
		if (id >= size) {
			do {
				WRITE_FENCE_GUARD();
				while (size++ <= id) {
					components.Push(nullptr);
				}
			} while (false);
			components[id] = component;

			uint32_t mask = ENTITY_UNIQUE_COMPONENT_MASK_BEGIN << id;
			if (mask != 0) {
				Flag().fetch_or(mask);
			}

			InitializeComponent(engine, component);
		} else {
			Component* target = components[id];
			if (target == component) return;

			if (target != nullptr) {
				if (target->GetUnique() != component->GetUnique()) {
					WRITE_FENCE_GUARD();
					components.Push(target);
				} else {
					UninitializeComponent(engine, target);
				}
			}

			components[id] = component;
			InitializeComponent(engine, component);
		}
	} else if (component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_UNIQUE) {
		Unique unique = component->GetUnique();
		size_t size = components.GetCount();
		for (size_t i = 0; i < size; i++) {
			Component*& c = components[i];
			if (c == nullptr) continue;
			if (c == component) return; // self

			if (c->GetUnique() == unique) {
				UninitializeComponent(engine, c);
				c = component;
				InitializeComponent(engine, component);
				return;
			}
		}

		components.Push(component);
		InitializeComponent(engine, component);
	} else {
		if (Flag().load(std::memory_order_acquire) & ENTITY_STORE_NULLSLOT) {
			size_t size = components.GetCount();
			for (size_t i = 0; i < size; i++) {
				Component*& target = components[i];
				if (target == nullptr) {
					target = component;
					InitializeComponent(engine, component);
					return;
				}
			}

			Flag().fetch_and(~ENTITY_STORE_NULLSLOT, std::memory_order_release);
		}

		do {
			WRITE_FENCE_GUARD();
			components.Push(component);
		} while (false);

		InitializeComponent(engine, component);
	}
}

void Entity::RemoveComponent(Engine& engine, Component* component) {
	assert(component != nullptr);
	uint32_t id = component->GetQuickUniqueID();
	if (id != ~(uint32_t)0) {
		assert(component->Flag().load(std::memory_order_acquire) & TINY_UNIQUE);
		size_t size = components.GetCount();
		if (id < size) {
			Component*& target = components[id];
			if (target == component) {
				uint32_t mask = ENTITY_UNIQUE_COMPONENT_MASK_BEGIN << id;
				if (mask != 0) {
					Flag().fetch_and(~mask);
				}

				UninitializeComponent(engine, target);
				target = nullptr;

				Flag().fetch_or(ENTITY_STORE_NULLSLOT, std::memory_order_relaxed);
			}
		}
	} else {
		size_t size = components.GetCount();
		for (size_t i = 0; i < size; i++) {
			Component*& target = components[i];
			if (target == component) {
				UninitializeComponent(engine, target);

				if (i == size - 1) {
					WRITE_FENCE_GUARD();
					while (i != 0 && components[i - 1] == nullptr) {
						--i;
						components.Pop();
					}
				} else {
					target = nullptr;
					Flag().fetch_or(ENTITY_STORE_NULLSLOT, std::memory_order_relaxed);
				}

				break;
			}
		}
	}
}

void Entity::UpdateEntityFlags() {
	FLAG p = 0, m;
	FLAG f = 0;
	size_t size = components.GetCount();
	for (size_t i = 0; i < size; i++) {
		Component* component = components[i];
		if (component != nullptr) {
			f |= components[i]->GetEntityFlagMask();
		}
	}

	do {
		p = Flag().load(std::memory_order_acquire);
		m = (p & ~ENTITY_HAS_ALL) | f;
	} while (!flag.compare_exchange_weak(p, m, std::memory_order_relaxed));
}

Component* Entity::GetUniqueComponent(Unique unique) const {
	READ_FENCE_GUARD();
	size_t size = components.GetCount();
	for (size_t i = 0; i < size; i++) {
		Component* component = components[i];
		if (component != nullptr && component->GetUnique()->IsClass(unique)) {
			assert(component->Flag().load(std::memory_order_acquire) & Tiny::TINY_UNIQUE);
			return component;
		}
	}

	return nullptr;
}

void Entity::ClearComponents(Engine& engine) {
	Deactivate(engine);

	if (Flag().load(std::memory_order_acquire) & ENTITY_HAS_TACH_EVENT) {
		size_t size = components.GetCount();
		for (size_t i = 0; i < size; i++) {
			Component* component = components[i];
			if (component != nullptr) {
				Event event(engine, Event::EVENT_DETACH_COMPONENT, this, component);
				PostEvent(event, ENTITY_HAS_TACH_EVENT);
			}
		}
	}

	Container temp;
	temp.Swap(components);

	size_t size = temp.GetCount();
	for (size_t i = 0; i < size; i++) {
		Component* component = temp[i];
		if (component != nullptr) {
			UninitializeComponent(engine, component);
		}
	}
}

void Entity::PostEvent(Event& event, FLAG mask) {
	READ_FENCE_GUARD();

	size_t size = components.GetCount();
	for (size_t i = 0; i < size; i++) {
		Component* component = components[i];
		if (component != nullptr && (component->GetEntityFlagMask() & mask)) {
			component->DispatchEvent(event, this);
		}
	}
}

TObject<IReflect>& Entity::operator () (IReflect& reflect) {
	// BaseClass::operator () (reflect);
	ReflectClass(Entity)[ReflectInterface(Unit)];

	if (reflect.IsReflectProperty()) {
		ReflectProperty(components);
	}

	return *this;
}

bool Entity::IsOrphan() const {
	return !!(Flag().load(std::memory_order_acquire) & ENTITY_STORE_ENGINE);
}

void Entity::UpdateBoundingBox(Engine& engine, bool recursive) {
	if (Flag().load(std::memory_order_relaxed) & TINY_PINNED)
		return;

	Float3Pair box(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	// Since Transform component is always the first component (if exists)
	// We iterate components from end to begin
	for (size_t i = components.GetCount(); i > 0; i--) {
		Component* component = components[i - 1];
		if (component != nullptr) {
			assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
			assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
			assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);
			component->UpdateBoundingBox(engine, box, recursive);
			assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
			assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
			assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);
		}
	}

	if (box.second.x() - box.first.x() >= 0) {
		assert(box.first.x() > -FLT_MAX && box.second.x() < FLT_MAX);
		assert(box.first.y() > -FLT_MAX && box.second.y() < FLT_MAX);
		assert(box.first.z() > -FLT_MAX && box.second.z() < FLT_MAX);

		GetKey() = box;
	}
}

void Entity::Activate(Engine& engine) {
	if (!(Flag().fetch_or(TINY_ACTIVATED) & TINY_ACTIVATED)) {
		if (Flag().load(std::memory_order_acquire) & ENTITY_HAS_ACTIVE_EVENT) {
			Event event(engine, Event::EVENT_ENTITY_ACTIVATE, this, nullptr);
			PostEvent(event, ENTITY_HAS_ACTIVE_EVENT);
		}
	}
}

void Entity::Deactivate(Engine& engine) {
	if (Flag().fetch_and(~TINY_ACTIVATED) & TINY_ACTIVATED) {
		if (Flag().load(std::memory_order_acquire) & ENTITY_HAS_ACTIVE_EVENT) {
			Event event(engine, Event::EVENT_ENTITY_DEACTIVATE, this, nullptr);
			PostEvent(event, ENTITY_HAS_ACTIVE_EVENT);
		}
	}
}
