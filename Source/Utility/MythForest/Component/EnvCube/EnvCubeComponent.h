// EnvCubeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	class EnvCubeComponent : public TAllocatedTiny<EnvCubeComponent, RenderableComponent> {
	public:
		EnvCubeComponent();
		~EnvCubeComponent() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		FLAG GetEntityFlagMask() const override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) override;
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;

		TShared<TextureResource> cubeMapTexture; // pre-filterred specular texture
		TShared<TextureResource> skyMapTexture; // irrandiance map texture

		Float3 range;
		float strength;
	};
}

