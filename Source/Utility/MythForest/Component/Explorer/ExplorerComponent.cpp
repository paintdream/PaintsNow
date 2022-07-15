#include "ExplorerComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"

using namespace PaintsNow;

ExplorerComponent::Proxy::Proxy(Component* c, const ProxyConfig& conf) : component(c), config(conf) {}

bool ExplorerComponent::Proxy::operator < (const ExplorerComponent::Proxy& rhs) const {
	return component < rhs.component;
}

ExplorerComponent::ExplorerComponent(Unique id) : identifier(id) {}
ExplorerComponent::~ExplorerComponent() {}

void ExplorerComponent::SetProxyConfig(Component* component, const ProxyConfig& config) {
	BinaryInsert(proxies, Proxy(component, config));
}

Unique ExplorerComponent::GetExploreIdentifier() const {
	return identifier;
}

void ExplorerComponent::SelectComponents(Engine& engine, Entity* entity, float refValue, std::vector<Component*, ComponentPointerAllocator>& activatedComponents) const {
	size_t size = entity->GetComponentCount();
	activatedComponents.reserve(size);
	uint32_t maxLayer = 0;

	for (size_t i = 0; i < size; i++) {
		Component* component = entity->GetComponent(i);
		if (component != nullptr) {
			std::vector<Proxy>::const_iterator it = BinaryFind(proxies.begin(), proxies.end(), component);
			if (it != proxies.end()) {
				if (refValue >= it->config.activateThreshold && refValue <= it->config.deactivateThreshold) {
					activatedComponents.emplace_back(component);
				}
			} else {
				activatedComponents.emplace_back(component); // always activated for unknown components
			}
		}
	}
}

