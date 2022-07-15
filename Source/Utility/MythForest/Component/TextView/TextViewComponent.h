// TextViewComponent.h
// PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component/Model/ModelComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/FontResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"
#include <sstream>

namespace PaintsNow {
	class TextViewComponent : public TAllocatedTiny<TextViewComponent, ModelComponent> {
	public:
		enum {
			TEXTVIEWCOMPONENT_SELECT_REV_COLOR = MODELCOMPONENT_CUSTOM_BEGIN,
			TEXTVIEWCOMPONENT_CURSOR_REV_COLOR = MODELCOMPONENT_CUSTOM_BEGIN << 1,
		};

		TextViewComponent(const TShared<FontResource>& fontResource, const TShared<MeshResource>& meshResource, const TShared<BatchComponent>& batchComponent);
		~TextViewComponent() override;
		uint32_t CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) override;
		void Initialize(Engine& engine, Entity* entity) override;
		void Uninitialize(Engine& engine, Entity* entity) override;
		void UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) override;

		class Element {
		public:
			Element(int16_t h, int16_t fs);
			int16_t totalWidth;
			int16_t yCoord;
			int16_t firstOffset;

			class Char {
			public:
				Char(int16_t c = 0, int16_t off = 0);
				int16_t xCoord;
				int16_t offset;
			};

			std::vector<Char> allOffsets;
		};

		int32_t Locate(Short2& rowCol, const Short2& pt, bool isPtRowCol) const;
		void SetText(Engine& engine, const String& text);
		void Scroll(const Short2& pt);
		void SetUpdateMark();

	public:
		TShared<FontResource> fontResource;
		uint32_t fontSize;
		String text;

	protected:
		class TagParser {
		public:
			class Node {
			public:
				enum TYPE { TEXT = 0, RETURN, COLOR, COLOR_CLOSED, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER };

				Node(TYPE t, uint32_t off, uint32_t len = 0) : type(t), offset(off), length(len) {}

				TYPE type;
				uint32_t offset;
				uint32_t length;
			};

			~TagParser() {
				Clear();
			}

			void Parse(const char* start, const char* end);
			void PushReturn(uint32_t offset);
			bool ParseAttrib(const char*& valueString, bool& isClose, const char* start, const char* end, const char* attrib);
			void PushFormat(uint32_t offset, const char* start, const char* end);
			void PushText(uint32_t offset, const char* start, const char* end);
			void Clear();

			std::vector<Node> nodes;
		};

		void SetSize(Engine& engine, const Short2& size);
		void UpdateRenderData(Engine& engine);

		uint32_t GetLineCount() const;
		void SetPasswordChar(int ch);
		bool IsEmpty() const;
		const Short2& GetSize() const;
		const Short2& GetFullSize() const;
		void SetPadding(const Short2& padding);

		class RenderInfo {
		public:
			RenderInfo(const Short2Pair& tr, const Short2Pair& pr, IRender::Resource* tex, const UChar4& c) : texRect(tr), posRect(pr), texture(tex), color(c) {}
			Short2Pair texRect;
			Short2Pair posRect;
			IRender::Resource* texture;
			UChar4 color;
		};

	protected:
		TagParser parser;

		std::vector<RenderInfo> renderInfos;
		std::vector<Element> lines;
		Short2 size;
		Short2 scroll;
		Short2 fullSize;
		Short2 padding;
		Short2 selectRange;
		UChar4 cursorColor;
		UChar4 selectColor;
		int32_t passwordChar;
		int32_t cursorChar;
		int32_t cursorPos;
	};
}

