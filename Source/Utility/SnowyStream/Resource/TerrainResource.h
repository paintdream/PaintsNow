// TerrainResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"
#include "../../../General/Interface/IAsset.h"
#include "TextureResource.h"

namespace PaintsNow {
	class TerrainResource : public TReflected<TerrainResource, RenderResourceBase> {
	public:
		TerrainResource(ResourceManager& manager, const String& uniqueID);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		void FromTexture(const TShared<TextureResource>& textureResource);

	protected:
		std::vector<float> terrainData;
		uint32_t width, height;
		float deltaHeight;
	};
}

