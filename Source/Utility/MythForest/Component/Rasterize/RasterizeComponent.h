// RasterizeComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	class RasterizeComponent : public TAllocatedTiny<RasterizeComponent, Component> {
	public:
		RasterizeComponent(const TShared<TextureResource>& texture);

		void RenderMesh(const TShared<MeshResource>& meshResource, const MatrixFloat4x4& transform);

	protected:
		TShared<TextureResource> targetTexture;
	};
}
