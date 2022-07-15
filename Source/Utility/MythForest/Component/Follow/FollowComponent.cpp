#include "FollowComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

FollowComponent::FollowComponent(uint32_t bufferSize, uint32_t delay) : currentTransformIndex(0), delayInterval(delay) {
	if (bufferSize > 0) {
		transformBuffer.resize(bufferSize);
		computeTransformBuffer.resize(bufferSize);
	}
}

FollowComponent::~FollowComponent() {}

void FollowComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
}

void FollowComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

TObject<IReflect>& FollowComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void FollowComponent::Attach(const TShared<TransformComponent>& transformComponent) {
	sourceTransformComponent = transformComponent;
}

Tiny::FLAG FollowComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT;
}

void FollowComponent::Interpolate(TransformComponent::TRSData& output, const TransformComponent::TRSData& lhs, const TransformComponent::TRSData& rhs, float t) {
	QuaternionFloat::Interpolate(output.rotation, lhs.rotation, rhs.rotation, t);
	output.scale = Math::Interpolate(lhs.scale, rhs.scale, t);
	output.translation = Math::Interpolate(lhs.translation, rhs.translation, t);
}

void FollowComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (event.eventID == Event::EVENT_TICK) {
		// Sample transform component
		if (sourceTransformComponent) {
			Record& record = transformBuffer[currentTransformIndex];
			sourceTransformComponent->GetTRS(record.trs);
			Engine& engine = event.engine;
			record.timestamp = engine.GetFrameTimestamp();

			TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
			if (transformComponent != nullptr) {
				// Bezier interpolation
				uint32_t count = verify_cast<uint32_t>(transformBuffer.size());
				if (count > 1) {
					Engine& engine = event.engine;
					uint32_t startTransformIndex = (currentTransformIndex + count - 1) % count;
					uint64_t now = engine.GetFrameTimestamp();
					uint64_t upperBound = transformBuffer[currentTransformIndex].timestamp;
					uint64_t lowerBound = transformBuffer[startTransformIndex].timestamp;
					float t = Math::Clamp(((float)(now - Math::Min(lowerBound, now)) - (float)delayInterval) / Math::Max(upperBound - lowerBound, (uint64_t)1), 0.0f, 1.0f);

					for (uint32_t i = startTransformIndex; i < startTransformIndex + count; i++) {
						computeTransformBuffer[i - startTransformIndex] = transformBuffer[i % count].trs;
					}

					for (uint32_t k = count - 1; k != 0; k--) {
						for (uint32_t i = 0; i < k; i++) {
							Interpolate(computeTransformBuffer[i], computeTransformBuffer[i], computeTransformBuffer[i + 1], t);
						}
					}

					transformComponent->SetTRS(computeTransformBuffer[0]);
				}
			}

			currentTransformIndex = (currentTransformIndex + 1) % transformBuffer.size();
		}
	}
}

