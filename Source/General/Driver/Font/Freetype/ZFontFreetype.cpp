#include "ZFontFreetype.h"
#include <cassert>
#include <map>

#ifdef FT2_BUILD_LIBRARY
#undef FT2_BUILD_LIBRARY
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#include <ft2build.h>
#include <freetype/ftglyph.h>
#else
#include "Core/include/ft2build.h"
#include "Core/include/ftglyph.h"
#endif

using namespace PaintsNow;

class FontFreetypeImpl : public IFontBase::Font {
public:
	uint8_t* buffer;
	size_t fontSize;
	FT_Library lib;
	FT_Face face;
};


void ZFontFreetype::Close(Font* font) {
	FontFreetypeImpl* impl = static_cast<FontFreetypeImpl*>(font);
	if (impl->buffer != nullptr) {
		delete[] impl->buffer;

		if (impl->face != nullptr) {
			FT_Done_Face(impl->face);
		}

		if (impl->lib != nullptr) {
			FT_Done_FreeType(impl->lib);
		}

		delete impl;
	}
}

IFontBase::Font* ZFontFreetype::Load(IStreamBase& stream, size_t length) {
	// Read a memory block
	uint8_t* buffer = new uint8_t[length];
	if (!stream.Read(buffer, length)) {
		delete[] buffer;
		return nullptr;
	}

	FontFreetypeImpl* impl = new FontFreetypeImpl();
	impl->buffer = buffer;
	impl->fontSize = 0;

	FT_Init_FreeType(&impl->lib);
	FT_New_Memory_Face(impl->lib, buffer, (FT_Long)length, 0, &impl->face);

//	const int DPI = 96;
//	FT_Set_Char_Size(face, ppx, ppx, DPI, DPI);
//	bitmapSize = 14;
	FT_Face face = impl->face;

	// scan for all available encodings
	const char* targetEnc = "UCS-2";
	// size_t selected = 0;
	for (size_t i = 0; i < (size_t)face->num_charmaps; i++) {
		const FT_CharMapRec* cm = face->charmaps[i];
		if (cm->encoding == FT_ENCODING_UNICODE) { // unicode
			targetEnc = "UTF-16";
			// selected = i;
			break;
		} else if (cm->encoding == FT_ENCODING_GB2312) {
			targetEnc = "GB2312";
			// selected = i;
			break;
		} else if (cm->encoding == FT_ENCODING_BIG5) {
			targetEnc = "BIG5";
			// selected = i;
			break;
		}
	}

//	FT_Set_Charmap(face, face->charmaps[selected]);
//	if(m_ansi)
//		FT_Select_Charmap(*(reinterpret_cast<FT_Face*>(m_face)), FT_ENCODING_GB2312);
//	else
	// FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	FT_Set_Charmap(impl->face, face->charmaps[0]);
	FT_Select_Charmap(impl->face, FT_ENCODING_UNICODE);

	return impl;
}

IFontBase::CHARINFO ZFontFreetype::RenderTexture(Font* font, String& texture, FONTCHAR character, size_t fontSize, float hinting) const {
	FontFreetypeImpl* impl = static_cast<FontFreetypeImpl*>(font);
	if (impl->fontSize != fontSize) {
		impl->fontSize = fontSize;
		FT_Set_Pixel_Sizes(impl->face, (FT_UInt)fontSize, (FT_UInt)fontSize);
	}

	// character = 0x4f5c;
	FT_Face face = impl->face;
	size_t index = FT_Get_Char_Index(face, (FT_ULong)character);
	FT_Glyph glyph;
	FT_Load_Glyph(face, (FT_UInt)index, FT_LOAD_DEFAULT);
	FT_Get_Glyph(face->glyph, &glyph);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
	FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
	FT_BitmapGlyph& bitmap_glyph = (FT_BitmapGlyph&)glyph;
	FT_Bitmap& bitmap = bitmap_glyph->bitmap;

	const int size = bitmap.width * bitmap.rows;
	texture.resize(size);
	uint8_t* data = (uint8_t*)texture.data();
	memcpy(data, bitmap.buffer, size);

	IFontBase::CHARINFO info;
	info.height = verify_cast<uint16_t>(bitmap.rows);
	info.width = verify_cast<uint16_t>(bitmap.width);
	info.adv.x() = verify_cast<uint16_t>(face->glyph->advance.x / 64);
	// info.adv.x() = face->glyph->metrics.horiAdvance;
	info.adv.y() = verify_cast<uint16_t>(face->glyph->advance.y / 64);
	info.bearing.x() = verify_cast<int16_t>(face->glyph->metrics.horiBearingX / 64);
	info.bearing.y() = verify_cast<int16_t>(face->glyph->metrics.horiBearingY / 64);
	info.delta.x() = verify_cast<int16_t>(bitmap_glyph->left);
	info.delta.y() = verify_cast<int16_t>(bitmap_glyph->top - info.height);

	FT_Done_Glyph(glyph);
	return info;
}
