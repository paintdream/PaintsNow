#include "TerrainComponent.h"

using namespace PaintsNow;

TerrainComponent::TerrainComponent() {}

uint32_t TerrainComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	return 0;
}
