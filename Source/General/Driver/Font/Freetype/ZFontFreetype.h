// ZFontFreetype.h
// PaintDream (paintdream@paintdream.com)
// 2014-12-16
//

#pragma once
#include "../../../Interface/IFontBase.h"

namespace PaintsNow {
	class ZFontFreetype final : public IFontBase {
	public:
		Font* Load(IStreamBase& stream, size_t length) override;
		void Close(Font* font) override;
		CHARINFO RenderTexture(Font* font, String& data, FONTCHAR character, size_t bitmapSiz, float hinting) const override;
	};
}
