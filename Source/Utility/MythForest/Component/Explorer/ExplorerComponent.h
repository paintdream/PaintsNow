// ExplorerComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../../../Core/Template/TCache.h"
#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"

namespace PaintsNow {
	// Level of details controller
	class ExplorerComponent : public TAllocatedTiny<ExplorerComponent, UniqueComponent<Component, SLOT_EXPLORER_COMPONENT> > {
	public:
		ExplorerComponent(Unique identifier);
		~ExplorerComponent() override;

		struct ProxyConfig {
			float activateThreshold;
			float deactivateThreshold;
		};

		void SetProxyConfig(Component* component, const ProxyConfig& config);
		Unique GetExploreIdentifier() const;
		typedef TCacheAllocator<Component*, uint8_t> ComponentPointerAllocator;
		void SelectComponents(Engine& engine, Entity* entity, float refValue, std::vector<Component*, ComponentPointerAllocator>& collectedComponents) const;

	protected:
		class Proxy {
		public:
			Proxy(Component* c = nullptr, const ProxyConfig& config = ProxyConfig());

			operator Component* () const {
				return component;
			}

			bool operator < (const Proxy& rhs) const;

			Component* component;
			ProxyConfig config;
		};

		std::vector<Proxy> proxies;
		Unique identifier;
	};
}

