// FieldTexture.h
// PaintDream (paintdream@paintdream.com)
// 2020-3-17
//

#pragma once
#include "../FieldComponent.h"
#include "../../../../SnowyStream/Resource/TextureResource.h"

namespace PaintsNow {
	class FieldTexture : public FieldComponent::FieldBase {
	public:
		FieldTexture(TShared<TextureResource> textureResource, const Float3Pair& range);
		virtual ~FieldTexture();
		Bytes operator [] (const Float3& position) const override;

		enum TEXTURE_TYPE {
			TEXTURE_2D, TEXTURE_3D,
		};

	protected:
		TShared<TextureResource> textureResource;
		Float3Pair range;
	};
}

