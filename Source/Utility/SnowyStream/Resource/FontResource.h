// FontResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#pragma once
#include "RenderResourceBase.h"
#include "TextureResource.h"
#include "../../../General/Interface/IFontBase.h"
#include "../../../Core/Template/TTagged.h"
#include "../../../Core/Template/TMap.h"

namespace PaintsNow {
	class FontResource : public TReflected<FontResource, DeviceResourceBase<IFontBase> >, public IDataUpdater {
	public:
		FontResource(ResourceManager& manager, const String& uniqueID);
		~FontResource() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) override;

		void Refresh(IFontBase& font, void* deviceContext) override;
		void Upload(IFontBase& font, void* deviceContext) override;
		void Download(IFontBase& font, void* deviceContext) override;
		void Attach(IFontBase& font, void* deviceContext) override;
		void Detach(IFontBase& font, void* deviceContext) override;
		uint32_t Update(IRender& render, IRender::Queue* queue) override;
		uint16_t GetFontTextureSize() const;

		class Char {
		public:
			Char() { memset(this, 0, sizeof(*this)); }

			IFontBase::CHARINFO info;
			Short2Pair rect;
			IRender::Resource* textureResource;
		};

		const Char& Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size);

	protected:
		class Slice {
		public:
			Slice(uint16_t size = 0, uint16_t dim = 0);
			const Char& Get(IRender& render, IRender::Queue* queue, IFontBase& font, IFontBase::FONTCHAR ch);
			void Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);

			friend class FontResource;

		protected:
			typedef std::map<IFontBase::FONTCHAR, Char> FontMap;

			uint32_t critical;
			FontMap cache;
			Short2Pair lastRect;
			Bytes buffer;
			uint16_t dim;
			uint16_t fontSize;
			IFontBase::Font* font;
			std::vector<TTagged<IRender::Resource*, 2> > cacheTextures;

			Short2 GetTextureSize() const;
			uint32_t UpdateFontTexture(IRender& render, IRender::Queue* queue);
			Short2Pair AllocRect(IRender& render, IRender::Queue* queue, const Short2& size);
		};

	private:
		std::map<uint32_t, Slice> sliceMap;
		IFontBase::Font* font;
		String rawFontData;
		uint16_t dim;
		uint16_t weight;
	};
}
