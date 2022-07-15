// IFontBase.h -- Font interfaces
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once

#include "../../Core/PaintsNow.h"
#include "../../Core/Template/TMatrix.h"
#include "../../Core/Interface/IStreamBase.h"
#include "IRender.h"
#include "../../Core/Interface/IDevice.h"

#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef unsigned __int64 uint64_t;
#else
#include <cstdint>
#endif

namespace PaintsNow {
	class IStreamBase;
	class pure_interface IFontBase : public IDevice {
	public:
		typedef uint32_t FONTCHAR;

		struct CHARINFO {
			uint16_t height;
			uint16_t width;
			Short2 adv;
			Short2 delta;
			Short2 bearing;
		};

		class Font {};

		~IFontBase() override;
		virtual Font* Load(IStreamBase& stream, size_t length) = 0;
		virtual void Close(Font* font) = 0;
		virtual CHARINFO RenderTexture(Font* font, String& data, FONTCHAR character, size_t bitmapSize, float hinting) const = 0;
	};
}

