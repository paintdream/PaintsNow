#include "FieldSimplygon.h"

using namespace PaintsNow;

FieldSimplygon::FieldSimplygon(SIMPOLYGON_TYPE t, const Float3Pair& b) : type(t), box(b) {

}

TObject<IReflect>& FieldSimplygon::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(type);
	}

	return *this;
}

Bytes FieldSimplygon::operator [] (const Float3& position) const {
	bool result = false;
	switch (type) {
		case BOUNDING_BOX:
			result = Math::Contain(box, position);
			break;
		case BOUNDING_SPHERE:
		{
			Float3 local = Math::ToLocal(box, position);
			result = Math::SquareLength(local) <= 1.0f;
		}
		break;
		case BOUNDING_CYLINDER:
		{
			Float3 local = Math::ToLocal(box, position);
			if (local.z() < -1.0f || local.z() > 1.0f) {
				result = false;
			} else {
				result = Math::SquareLength(Float2(local.x(), local.y())) <= 1.0f;
			}
		}
		break;
	}

	Bytes encoder;
	encoder.Assign((const uint8_t*)&result, sizeof(bool));
	return result;
}

template <int type, class Q>
class BoxQueryer {
public:
	BoxQueryer(Q& q, const Float3Pair& b) : queryer(q), box(b) {}
	Q& queryer;
	const Float3Pair& box;

	bool operator () (TKdTree<Float3Pair, Unit>& tree) const {
		if (type == FieldSimplygon::BOUNDING_BOX) {
			queryer(static_cast<Entity&>(tree));
		} else if (type == FieldSimplygon::BOUNDING_SPHERE) {
			Float3 from = Math::ToLocal(box, tree.GetKey().first);
			Float3 to = Math::ToLocal(box, tree.GetKey().second);

			float dx = Math::Max(0.0f, Math::Max(from.x(), -to.x()));
			float dy = Math::Max(0.0f, Math::Max(from.y(), -to.y()));
			float dz = Math::Max(0.0f, Math::Max(from.z(), -to.z()));

			if (dx * dx + dy * dy + dz * dz <= 1.0f) {
				queryer(static_cast<Entity&>(tree));
			}
		} else if (type == FieldSimplygon::BOUNDING_CYLINDER) {
			Float3 from = Math::ToLocal(box, tree.GetKey().first);
			Float3 to = Math::ToLocal(box, tree.GetKey().second);

			float dx = Math::Max(0.0f, Math::Max(from.x(), -to.x()));
			float dy = Math::Max(0.0f, Math::Max(from.y(), -to.y()));

			if (dx * dx + dy * dy <= 1.0f) {
				queryer(static_cast<Entity&>(tree));
			}
		}

		return true; // search all
	}
};

class Poster {
public:
	Poster(Event& e, Tiny::FLAG m) : event(e), mask(m) {}

	Event& event;
	Tiny::FLAG mask;

	void operator () (Entity& entity) {
		entity.PostEvent(event, mask);
	}
};

void FieldSimplygon::PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const {
	Poster poster(event, mask);
	switch (type) {
		case BOUNDING_BOX:
		{
			BoxQueryer<BOUNDING_BOX, Poster> q(poster, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
		case BOUNDING_SPHERE:
		{
			BoxQueryer<BOUNDING_SPHERE, Poster> q(poster, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
		case BOUNDING_CYLINDER:
		{
			BoxQueryer<BOUNDING_CYLINDER, Poster> q(poster, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
	}
}

class Collector {
public:
	Collector(std::vector<TShared<Entity> >& e) : entities(e) {}
	std::vector<TShared<Entity> >& entities;

	void operator () (Entity& entity) {
		entities.push_back(&entity);
	}
};

void FieldSimplygon::QueryEntitiesForEntityTree(Entity* entity, std::vector<TShared<Entity> >& entities) const {
	Collector collector(entities);
	switch (type) {
		case BOUNDING_BOX:
		{
			BoxQueryer<BOUNDING_BOX, Collector> q(collector, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
		case BOUNDING_SPHERE:
		{
			BoxQueryer<BOUNDING_SPHERE, Collector> q(collector, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
		case BOUNDING_CYLINDER:
		{
			BoxQueryer<BOUNDING_CYLINDER, Collector> q(collector, box);
			entity->Query(std::true_type(), box, q);
			break;
		}
	}
}
