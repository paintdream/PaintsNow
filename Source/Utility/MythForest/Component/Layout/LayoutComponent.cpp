#include "LayoutComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../../Entity.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include <queue>

using namespace PaintsNow;

LayoutComponent::LayoutComponent() : hostEntity(nullptr) {
	runningLayoutCount.store(0, std::memory_order_relaxed);
}

LayoutComponent::~LayoutComponent() {}

void LayoutComponent::SetUpdateMark() {
	Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_acquire);
}

Entity* LayoutComponent::GetHostEntity() const {
	return hostEntity;
}

// Layouts
static inline void CheckRect(const Float2Pair& rect) {
	assert(rect.first.x() >= 0);
	assert(rect.first.y() >= 0);
	assert(rect.second.x() >= 0);
	assert(rect.second.y() >= 0);
}

template <class T, class A>
inline Float2 CalcDimension(LayoutComponent* win, float& lastMargin, float& sum, float& range, int& count, A& margin, A& padding, A& size, T get) {
	Float2 offset;
	float value = get(size.first);
	float maxvalue = get(size.second);
	float m = get(margin.first);
	assert(m >= 0 && value >= 0);
	if (win->Flag().load(std::memory_order_relaxed) & LayoutComponent::LAYOUT_ADAPTABLE) {
		assert(false); // not fully implemeneted.
		CheckRect(win->rect);
		win->rect.second = win->rect.first;
		// win->DoLayout(engine); // TODO: Async wait.
		CheckRect(win->rect);
		maxvalue = value = get(win->scrollSize) + get(win->rect.second) - get(win->rect.first);
		assert(value >= 0);
	}

	if (maxvalue - value >= 0) {
		m = (lastMargin == -1 ? 0.0f : Math::Max(m, lastMargin)) + get(padding.first);
		assert(m >= 0);
		assert(value >= 0);
		sum += maxvalue - value;
		offset.x() = m;
		offset.y() = m + value;
		range -= (value + get(padding.first) + get(padding.second) + m);
		count++;
		lastMargin = get(margin.second);
		assert(lastMargin >= 0);
	} else {
		// hide element?
		lastMargin = Math::Max(lastMargin, get(margin.second));
		assert(lastMargin >= 0);
	}

	return offset;
}

template <class Getter>
class CompareRatio {
public:
	CompareRatio() {}
	bool operator () (LayoutComponent* lhs, LayoutComponent* rhs) const {
		Getter get;
		assert(lhs->weight != 0 && rhs->weight != 0);
		float left = (get(lhs->size.second) - get(lhs->size.first)) * rhs->weight, right = (get(rhs->size.second) - get(rhs->size.first)) * lhs->weight;
		if (get(lhs->size.second) == 0) {
			if (get(rhs->size.second) == 0) {
				return false;
			} else {
				return true;
			}
		} else if (get(rhs->size.second) == 0) {
			return false;
		} else {
			return left > right;
		}
	}
};

template <class Getter, class Other>
struct DoLayoutImpl {
public:
	void DoLayout(LayoutComponent* widget, std::vector<LayoutComponent*> components) {
		// No entity, no layout
		if (components.empty()) return;

		// two getters, one gets main direction data, and the other one gets the remaining
		Getter get;
		Other other;

		// valid size
		const Float2Pair& range = widget->rect;
		CheckRect(range);

		uint32_t winCount = widget->count;
		const uint32_t start = widget->start;
		Tiny::FLAG flag = widget->Flag().load();

		Float2 t = range.second - range.first;
		Float2 scrollSize = t;

		int count = 0;
		float margin = -1;
		float size = get(t);
		float sum = 0;
		int weight = 0;
		size_t num = 0;
		if (winCount == -1) {
			winCount = verify_cast<uint32_t>(components.size());
		}

		Float2 current = range.first;
		static CompareRatio<Getter> cmp;
		int valid = 0;
		std::priority_queue<LayoutComponent*, std::vector<LayoutComponent*>, CompareRatio<Getter> > sorted(cmp);
		const Tiny::FLAG checkFlag = LayoutComponent::LAYOUT_DISPLAY | Tiny::TINY_PINNED;

		for (num = 0; num < components.size(); num++) {
			LayoutComponent* win = components[num];
			if ((int)num < start) {
				continue;
			} else if ((int)num >= start + winCount) {
				break;
			} else {
				Float2Pair& rect = win->rect;
				CheckRect(rect);

				if ((win->Flag().load(std::memory_order_relaxed) & checkFlag) == checkFlag) {
					Float2 pair = CalcDimension(win, margin, sum, size, count, win->margin, win->padding, win->size, Getter());
					assert(pair.x() >= 0 && pair.y() >= 0);
					if (get(win->size.second) - get(win->size.first) >= 0) {
						if (!(win->Flag().load(std::memory_order_relaxed) & LayoutComponent::LAYOUT_ADAPTABLE)) {
							sorted.push(win);
							weight += win->weight;
							valid++;
						}

						assert(get(current) >= 0);
						get(current) += get(win->padding.first);
						assert(get(current) >= 0);
						rect.first = rect.second = current;

						CheckRect(rect);

						get(rect.first) += pair.x();
						get(rect.second) += pair.y();
						get(current) += pair.y() + get(win->padding.second);
						// win->remained = (float)valid;

						Float2Pair saveRange = range;
						other(rect.first) = other(saveRange.first) + other(win->padding.first);
						other(rect.second) = other(saveRange.second) - other(win->padding.second);
						if (other(win->size.second) - other(win->size.first) >= 0 && other(win->size.second) > 0) {
							other(rect.second) = Math::Min(Math::Max(other(rect.second), other(rect.first) + other(win->size.first)), other(rect.first) + other(win->size.second));

							// Update scroll size, important!
							other(scrollSize) = Math::Max(other(scrollSize), other(rect.second) - other(range.first));
						}

						CheckRect(rect);
					} else {
						get(rect.first) = get(rect.second) = 0;
					}
				} else {
					// move
					rect.first = range.first;

					Float2 s = win->size.second;
					for (int i = 0; i < 2; i++) {
						if (s[i] == 0) {
							rect.second[i] = range.second[i];
						} else {
							rect.second[i] = range.first[i] + s[i];
						}
					}
					rect.first += win->padding.first;
					rect.second -= win->padding.second;

					CheckRect(rect);
				}
			}
		}

		while (size > 0 && !sorted.empty()) {
			LayoutComponent* top = sorted.top();
			// check!
			assert(get(top->size.second) - get(top->size.first) >= 0);
			if (get(top->size.second) != 0 && size * top->weight > ((get(top->size.second) - get(top->size.first)) * weight)) {
				float diff = get(top->size.second) - get(top->size.first);

				assert(diff >= 0);
				assert(size >= diff);
				size -= diff;
				get(top->rect.second) += diff;
				top->remained = diff;
				sorted.pop();
				weight -= top->weight;

				CheckRect(top->rect);
			} else {
				break;
			}
		}

		get(scrollSize) = Math::Max(0.0f, get(scrollSize) - size);
		other(scrollSize) = Math::Max(0.0f, other(scrollSize) - (other(range.second) - other(range.first)));
		// int totalHeight = range.first.y() + range.second.y();// +scrollSize.y();

		// allocate remaining
		if (size < 0) {
			size = 0;
		}

		while (!sorted.empty()) {
			LayoutComponent* top = sorted.top();
			float diff = size * top->weight / weight;
			if (get(top->size.second) - get(top->size.first) >= 0 && get(top->size.second) > 0) {
				diff = Math::Min(diff, get(top->size.second) - get(top->rect.second) + get(top->rect.first));
			}
			assert(diff >= 0);
			get(top->rect.second) += diff;
			top->remained = diff;
			sorted.pop();
			CheckRect(top->rect);
		}

		float remained = 0;
		for (size_t k = 0; k < components.size(); k++) {
			LayoutComponent* win = components[k];
			if ((win->Flag().load(std::memory_order_relaxed) & checkFlag) == checkFlag) {
				assert(win->remained >= 0);
				Float2Pair& rect = win->rect;
				get(rect.first) += remained;
				get(rect.second) += remained;
				remained += win->remained;
				CheckRect(rect);
				// last = get(rect.second);

				// printf("Rect (%d, %d, %d, %d)\n", rect.first.x(), rect.first.y(), rect.second.x(), rect.second.y());
				// add padding ...
				CheckRect(rect);

				win->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_acquire);
			}

			CheckRect(win->rect);
		}

		widget->scrollSize = scrollSize;

	}
};

template <size_t k>
struct GetK {
	float& operator () (Float2& val) {
		return val[k];
	}
	const float operator () (const Float2& val) const {
		return val[k];
	}
};

typedef GetK<0> GetX;
typedef GetK<1> GetY;

static inline void GetAllLayoutComponents(std::vector<Entity*>& entities, std::vector<LayoutComponent*>& components, Entity* root) {
	while (root != nullptr) {
		LayoutComponent* component = root->GetUniqueComponent(UniqueType<LayoutComponent>());
		if (component != nullptr) {
			components.emplace_back(component);
		}

		entities.emplace_back(root);
		root->ReferenceObject();

		GetAllLayoutComponents(entities, components, static_cast<Entity*>(root->Right()));
		root = root->Left();
	}
}

void LayoutComponent::UpdateTransform(const MatrixFloat4x4& transform) {
	TransformComponent* transformComponent = hostEntity->GetUniqueComponent(UniqueType<TransformComponent>());

	MatrixFloat4x4 cpy = transform;
	for (size_t j = 0; j < 2; j++) {
		cpy.data[j][0] *= (rect.second.x() - rect.first.x()) / 2;
		cpy.data[j][1] *= (rect.second.y() - rect.first.y()) / 2;
	}

	cpy.data[3][0] += (rect.first.x() + rect.second.x()) / 2;
	cpy.data[3][1] += (rect.first.y() + rect.second.y()) / 2;

	transformComponent->SetTransform(transform);

	// TODO: update bounding box
}

void LayoutComponent::RoutineLayoutForSpaceComponent(Engine& engine, const TShared<SpaceComponent>& spaceComponent, const MatrixFloat4x4& transform) {
	// should copy 'this' content before updating?
	std::vector<LayoutComponent*> components;
	std::vector<Entity*> entities;

	GetAllLayoutComponents(entities, components, spaceComponent->GetRootEntity());
	spaceComponent->RemoveAll(engine);

	if (Flag().load(std::memory_order_relaxed) & LAYOUT_VERTICAL) {
		static DoLayoutImpl<GetY, GetX> impl;
		impl.DoLayout(this, components);
	} else {
		static DoLayoutImpl<GetX, GetY> impl;
		impl.DoLayout(this, components);
	}

	for (size_t i = 0; i < components.size(); i++) {
		components[i]->UpdateTransform(transform);
	}

	for (size_t j = 0; j < entities.size(); j++) {
		Entity* entity = entities[j];
		spaceComponent->Insert(engine, entity);
		entity->ReleaseObject();
	}

	if (runningLayoutCount.fetch_sub(1, std::memory_order_relaxed) == 1) {
		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}

void LayoutComponent::DoLayout(Engine& engine, const MatrixFloat4x4& transform) {
	if ((Flag().load(std::memory_order_acquire) & TINY_MODIFIED)) {
		// Collect Sub LayoutComponents
		assert(hostEntity != nullptr);
		size_t size = hostEntity->GetComponentCount();

		for (size_t i = 0; i < size; i++) {
			Component* component = hostEntity->GetComponent(i);
			if (component != nullptr && component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				// dispatch collection ...
				runningLayoutCount.fetch_add(1, std::memory_order_acquire);
				if (spaceComponent->GetWarpIndex() == engine.GetKernel().GetCurrentWarpIndex()) {
					RoutineLayoutForSpaceComponent(engine, spaceComponent, transform);
				} else {
					spaceComponent->QueueRoutine(engine, CreateTaskContextFree(Wrap(this, &LayoutComponent::RoutineLayoutForSpaceComponent), std::ref(engine), spaceComponent, transform));
				}
			}
		}
	}
}

void LayoutComponent::Initialize(Engine& engine, Entity* entity) {
	assert(hostEntity == nullptr);
	hostEntity = entity;
}

void LayoutComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert(hostEntity != nullptr);
	hostEntity = nullptr;
}