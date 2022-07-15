#include "TextViewComponent.h"
#include "../../MythForest.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include <utility>

using namespace PaintsNow;

static inline int GetUtf8Size(char c) {
	int t = 1 << 7;
	int r = c;
	int count = 0;
	while (r & t) {
		r = r << 1;
		count++;
	}

	return count == 0 ? 1 : count;
}

void TextViewComponent::TagParser::Parse(const char* start, const char* end) {
	Clear();

	bool less = false;
	const char* q = nullptr;
	const char* p = nullptr;
	bool slash = false;
	for (p = start, q = start; p != end; ++p) {
		int i = GetUtf8Size(*p);
		if (i != 1) {
			p += i - 1;
			continue;
		}

		if (*p == '\\') {
			slash = true;
		} else {
			uint32_t offset = verify_cast<uint32_t>(q - start);
			if (*p == '<' && !slash) {
				PushText(offset, q, p);
				less = true;
				q = p + 1;
			} else if (*p == '>' && less && !slash) {
				assert(q != nullptr);
				PushFormat(offset, q, p);
				less = false;
				q = p + 1;
			} else if (*p == '\n' && !less) {
				PushText(offset, q, p);
				PushReturn(offset);
				q = p + 1;
			}

			slash = false;
		}
	}

	PushText(verify_cast<uint32_t>(q - start), q, p);
}

void TextViewComponent::TagParser::PushReturn(uint32_t offset) {
	nodes.emplace_back(Node(Node::RETURN, offset));
}

bool TextViewComponent::TagParser::ParseAttrib(const char*& valueString, bool& isClose, const char* start, const char* end, const char* attrib) {
	assert(attrib != nullptr);
	bool ret = false;
	valueString = nullptr;
	isClose = false;
	const uint32_t len = verify_cast<uint32_t>(strlen(attrib));
	const char* format = start;

	if ((size_t)(end - start) >= len && memcmp(format, attrib, len) == 0) {
		// find "="
		const char* t;
		for (t = format + len; *t != '\0'; ++t) {
			if (*t == '=')
				break;
		}

		while (*t != '\0' && !isalnum(*t)) t++;

		if (*t != '\0')
			valueString = start + (t - format);

		ret = true;
	} else if (format[0] == '/' && (size_t)(end - start) >= len + 1 && memcmp(format + 1, attrib, len) == 0) {
		isClose = true;
		ret = true;
	}

	return ret;
}

void TextViewComponent::TagParser::PushFormat(uint32_t offset, const char* start, const char* end) {
	const char* valueString;
	bool isClose;

	if (ParseAttrib(valueString, isClose, start, end, "color")) {
		if (isClose) {
			nodes.emplace_back(Node(Node::COLOR_CLOSED, offset));
		} else {
			if (valueString != nullptr) {
				nodes.emplace_back(Node(Node::COLOR, (uint32_t)(valueString - start) + offset));
			}
		}
	} else if (ParseAttrib(valueString, isClose, start, end, "align")) {
		Node::TYPE t = Node::ALIGN_LEFT;
		static const char* right = "right";
		static const char* center = "center";
		if ((size_t)(end - valueString) >= strlen(right) && memcmp(valueString, right, strlen(right)) == 0) {
			t = Node::ALIGN_RIGHT;
		} else if ((size_t)(end - valueString) >= strlen(center) && memcmp(valueString, center, strlen(center)) == 0) {
			t = Node::ALIGN_CENTER;
		}

		nodes.emplace_back(Node(t, (uint32_t)(valueString - start) + offset));
	}
}

void TextViewComponent::TagParser::PushText(uint32_t offset, const char* start, const char* end) {
	if (start != end) {
		uint32_t length = verify_cast<uint32_t>(end - start);
		nodes.emplace_back(Node(Node::TEXT, offset, length));
	}
}

void TextViewComponent::TagParser::Clear() {
	nodes.clear();
}

TextViewComponent::TextViewComponent(const TShared<FontResource>& font, const TShared<MeshResource>& mesh, const TShared<BatchComponent>& batch) : BaseClass(mesh, batch), fontResource(std::move(font)), passwordChar(0), cursorChar('|'), cursorPos(0), fontSize(24), size(256, 32), scroll(0, 0), padding(0, 0), fullSize(0, 0), selectRange(0, 0), cursorColor(255, 255, 255, 255), selectColor(0, 0, 0, 0) {
	Flag().fetch_or(RENDERABLECOMPONENT_CAMERAVIEW | TEXTVIEWCOMPONENT_CURSOR_REV_COLOR | TEXTVIEWCOMPONENT_SELECT_REV_COLOR, std::memory_order_relaxed);
}

void TextViewComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
}

void TextViewComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

TextViewComponent::~TextViewComponent() {}

const Short2& TextViewComponent::GetFullSize() const {
	return fullSize;
}

void TextViewComponent::SetPadding(const Short2& value) {
	padding = value;
}

void TextViewComponent::Scroll(const Short2& pt) {
	scroll = pt;
}

void TextViewComponent::SetText(Engine& engine, const String& t) {
	text = t;
	parser.Parse(text.data(), text.data() + text.size());

	UpdateRenderData(engine);
}

static int Utf8ToUnicode(const unsigned char* s, int size) {
	char uc[] = { 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03 };
	unsigned long data = 0;
	unsigned long tmp = 0;
	int count = 0;

	const unsigned char* p = s;
	while (count < size) {
		unsigned char c = (*p);
		tmp = c;

		if (count == 0) {
			tmp &= uc[size - 1];
		} else {
			tmp &= 0x3f;
		}

		tmp = tmp << (6 * (size - count - 1));
		data |= tmp;

		p++;
		count++;
	}

	return data;
}

TextViewComponent::Element::Element(int16_t h, int16_t s) : totalWidth(0), firstOffset(h), yCoord(0) {}
TextViewComponent::Element::Char::Char(int16_t c, int16_t off) : xCoord(c), offset(off) {}

void TextViewComponent::UpdateRenderData(Engine& engine) {
	if (!fontResource) {
		return;
	}

	// Update buffers
	std::vector<Float4> bufferData;
	IFontBase& fontBase = engine.interfaces.fontBase;
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = engine.snowyStream.GetRenderResourceManager()->GetWarpResourceQueue();
	renderInfos.clear();

	Short2 fullSize;
	int ws = size.x();
	int h = fontSize + padding.y();
	assert(h != 0);

	Short2 start = scroll;
	int count = 0;
	int currentWidth = -start.x();
	int maxWidth = 0;
	int currentHeight = -start.y();

	// if (currentWidth < 0) currentWidth = 0;
	// if (currentHeight < 0) currentHeight = 0;
	bool full = false;
	bool showCursor = false;
	bool selectRevColor = !!(Flag().load(std::memory_order_acquire) & TEXTVIEWCOMPONENT_SELECT_REV_COLOR);
	bool cursorRevColor = !!(Flag().load(std::memory_order_acquire) & TEXTVIEWCOMPONENT_CURSOR_REV_COLOR);
	int align = TagParser::Node::ALIGN_LEFT;
	Short2 texSize;
	// fontResource->GetFontTexture(render, queue, fontSize, texSize);

	const FontResource::Char& cursor = fontResource->Get(render, queue, fontBase, cursorChar, fontSize);
	Short2 current;
	const FontResource::Char& pwd = fontResource->Get(render, queue, fontBase, passwordChar, fontSize);
	UChar4 color(255, 255, 255, 255);
	lines.emplace_back(Element(currentHeight, 0));
	FontResource::Char info;

	for (size_t j = 0; j < parser.nodes.size(); j++) {
		const TagParser::Node& node = parser.nodes[j];
		if (node.type == TagParser::Node::TEXT) {
			int step = 1;
			for (const char* p = text.data() + node.offset; p < text.data() + node.offset + node.length; p += step) {
				// use utf-8 as default encoding
				step = GetUtf8Size(*p);
				info = passwordChar != 0 ? pwd : fontResource->Get(render, queue, fontBase, Utf8ToUnicode((const unsigned char*)p, step), fontSize);

				int temp = info.info.adv.x() + padding.x();
				currentWidth += temp;
				maxWidth = maxWidth > currentWidth ? maxWidth : currentWidth;

				if (currentHeight > size.y() - h) {
					full = true;
					//	break;
				}

				if (temp >= ws) {
					break;
				}

				if (currentWidth + start.x() >= ws) {
					lines.emplace_back(Element(currentHeight, node.offset));
					currentWidth = -start.x();
					currentHeight += h;
					p -= step;

					count++;
					continue;
				}

				int16_t offset = (int16_t)(p - text.data());
				lines.back().allOffsets.emplace_back(Element::Char(currentWidth - temp / 2, offset));
				lines.back().totalWidth = currentWidth + start.x();

				if (!full && currentHeight >= 0) {
					int wt = info.info.width;
					int ht = info.info.height;
					int centerOffset = (temp - info.info.width) / 2;
					int alignOffset = (size_t)count >= this->lines.size() ? 0 : align == TagParser::Node::ALIGN_LEFT ? 0 : align == TagParser::Node::ALIGN_CENTER ? (ws - this->lines[count].totalWidth) / 2 : (ws - this->lines[count].totalWidth);
					current = Short2(currentWidth - temp + alignOffset + info.info.bearing.x(), currentHeight + (h - ht) - info.info.delta.y());

					UChar4 c = color;
					if (selectRange.x() <= offset && selectRange.y() > offset) {
						if (selectRevColor) {
							c = UChar4(255, 255, 255, 255) - color;
							c.a() = color.a();
						} else {
							c = selectColor;
						}
					}

					Short2 end(current.x() + wt, current.y() + ht);
					renderInfos.emplace_back(RenderInfo(info.rect, Short2Pair(current, end), info.textureResource, c));

					// if cursor ?
					if (showCursor && cursorPos <= offset && cursorChar != 0) {
						current.y() += info.info.delta.y() - cursor.info.delta.y() + ht - cursor.info.height;
						// current.x() -= cursor.info.width;
						if (cursorRevColor) {
							c = UChar4(255, 255, 255, 255) - color;
						} else {
							c = cursorColor;
						}

						Short2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
						renderInfos.emplace_back(RenderInfo(cursor.rect, Short2Pair(current, m), info.textureResource, c));
					}
				}
			} // end for

			if (full) break;
		} else if (node.type == TagParser::Node::ALIGN_LEFT || node.type == TagParser::Node::ALIGN_RIGHT || node.type == TagParser::Node::ALIGN_CENTER) {
			align = node.type;
		} else if (node.type == TagParser::Node::RETURN) {
			lines.emplace_back(Element(start.x(), (int16_t)node.offset));
			currentWidth = -start.x();
			currentHeight += h;
			count++;
		} else if (node.type == TagParser::Node::COLOR) {
			unsigned int value = 0;
			sscanf(text.data() + node.offset, "%x", &value);
			color = UChar4(((value >> 16) & 0xff), ((value >> 8) & 0xff), (value & 0xff), 1);
		}
	}

	// if cursor ?
	if (!full && currentHeight >= 0 && showCursor && cursorChar != 0) {
		UChar4 c;
		const FontResource::Char& ch = fontResource->Get(render, queue, fontBase, Utf8ToUnicode((const unsigned char*)&cursorChar, GetUtf8Size(cursorChar >> 24)), fontSize);
		current.x() += info.info.width;
		current.y() += info.info.delta.y() - cursor.info.delta.y() + info.info.height - cursor.info.height;
		// current.x() -= cursor.info.width;
		if (cursorRevColor) {
			c = UChar4(255, 255, 255, 255) - color;
		} else {
			c = cursorColor;
		}

		Short2 m(current.x() + cursor.info.width, current.y() + cursor.info.height);
		renderInfos.emplace_back(RenderInfo(cursor.rect, Short2Pair(current, m), ch.textureResource, c));
	}

	count++;
	currentHeight += h;

	fullSize.x() = maxWidth + start.x();
	fullSize.y() = currentHeight + start.y();

	// do update
	fontResource->Update(render, queue);
}

void TextViewComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {
	Math::Union(box, Float3(-1.0f, -1.0f, -1.0f)); // TODO: be more precise
	Math::Union(box, Float3(1.0f, 1.0f, 1.0f));
}

uint32_t TextViewComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
	if (renderInfos.empty()) return 0;

	uint32_t start = verify_cast<uint32_t>(outputDrawCalls.size());
	uint32_t count = BaseClass::CollectDrawCalls(outputDrawCalls, inputRenderData, bytesCache, option);
	if (count == ~(uint32_t)0) return count;

	const Bytes texCoordRectKey = StaticBytes(texCoordRect);
	const Bytes instanceColorKey = StaticBytes(instanceColor);
	float invTexSize = 1.0f / fontResource->GetFontTextureSize();

	assert(inputRenderData.viewResolution.x() != 0 && inputRenderData.viewResolution.y() != 0);
	float invX = 1.0f / inputRenderData.viewResolution.x(), invY = 1.0f / inputRenderData.viewResolution.y();

	for (uint32_t i = start; i < count; i++) {
		OutputRenderData& renderData = outputDrawCalls[i];
		PassBase::Updater& updater = renderData.shaderResource->GetPassUpdater();
		IRender::Resource::QuickDrawCallDescription& drawCall = renderData.drawCallDescription;
		IRender::Resource::RenderStateDescription& renderState = renderData.renderStateDescription;
		renderState.stencilReplacePass = 0;
		renderState.cull = 1;
		renderState.fill = 1;
		renderState.colorWrite = 1;
		renderState.blend = 1;
		renderState.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
		renderState.depthWrite = 0;
		renderState.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		renderState.stencilWrite = 0;
		renderState.stencilMask = 0;
		renderState.stencilValue = 0;

		const PassBase::Parameter& paramMainTexture = updater[IShader::BindInput::MAINTEXTURE];
		const PassBase::Parameter& paramTexCoordRect = updater[texCoordRectKey];
		const PassBase::Parameter& paramInstanceColor = updater[instanceColorKey];
		assert(paramMainTexture && paramTexCoordRect && paramInstanceColor);

		// IRender::Resource* lastMainTexture = renderInfos[0].texture;
		drawCall.instanceCount = verify_cast<uint16_t>(renderInfos.size());

		// prepare instance buffer data
		Bytes localInstancedData;
		Bytes localInstancedColorData;

		Float4* localInstancedPtr;
		Float4* localInstancedColorPtr;

		size_t localInstancedStride;
		size_t localInstancedColorStride;

		if (paramInstanceColor.slot == paramTexCoordRect.slot) {
			localInstancedData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4) * renderInfos.size() * 2));
			localInstancedPtr = reinterpret_cast<Float4*>(localInstancedData.GetData());
			localInstancedColorPtr = reinterpret_cast<Float4*>(localInstancedData.GetData() + sizeof(Float4));
			localInstancedStride = sizeof(Float4) * 2;
			localInstancedColorStride = sizeof(Float4) * 2;
		} else {
			localInstancedData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4) * renderInfos.size()));
			localInstancedColorData = bytesCache.New(verify_cast<uint32_t>(sizeof(Float4) * renderInfos.size()));
			localInstancedPtr = reinterpret_cast<Float4*>(localInstancedData.GetData());
			localInstancedColorPtr = reinterpret_cast<Float4*>(localInstancedColorData.GetData());
			localInstancedStride = sizeof(Float4);
			localInstancedColorStride = sizeof(Float4);
		}

		IRender::Resource** textures = drawCall.GetTextures();
		renderData.localTransforms.reserve(renderInfos.size());

		for (size_t j = 0; j < renderInfos.size(); j++) {
			RenderInfo& renderInfo = renderInfos[j];
			/* TODO:
			if (renderInfo.texture != lastMainTexture) { // texture changed?
				outputDrawCalls.emplace_back(renderData);
				renderData.localInstancedData.clear();
				renderData.localTransforms.clear();
			}*/

			size_t k = paramMainTexture.slot;
			IRender::Resource*& texture = textures[k];
			texture = renderInfo.texture;
			Float4 texRect = Float4(
				renderInfo.texRect.first.x() * invTexSize,
				renderInfo.texRect.second.y() * invTexSize,
				renderInfo.texRect.second.x() * invTexSize,
				renderInfo.texRect.first.y() * invTexSize
			);

			UChar4 color = renderInfo.color;
			Float4 fcolor = Float4(color.r() / 255.0f, color.g() / 255.0f, color.b() / 255.0f, color.a() / 255.0f);

			float mat[16] = {
				(float)(renderInfo.posRect.second.x() - renderInfo.posRect.first.x()) * invX, 0, 0, 0,
				0, (float)(renderInfo.posRect.second.y() - renderInfo.posRect.first.y()) * invY, 0, 0,
				0, 0, 1, 0,
				(renderInfo.posRect.second.x() + renderInfo.posRect.first.x()) * invX, -(renderInfo.posRect.second.y() + renderInfo.posRect.first.y()) * invY, 0.0f, 1.0f
			};

			*localInstancedPtr = texRect;
			localInstancedPtr = reinterpret_cast<Float4*>(reinterpret_cast<uint8_t*>(localInstancedPtr) + localInstancedStride);
			*localInstancedColorPtr = fcolor;
			localInstancedColorPtr = reinterpret_cast<Float4*>(reinterpret_cast<uint8_t*>(localInstancedColorPtr) + localInstancedColorStride);

			renderData.localTransforms.emplace_back(MatrixFloat4x4(mat));
		}

		for (size_t k = 0; k < renderData.localInstancedData.size(); k++) {
			assert(renderData.localInstancedData[k].first != paramTexCoordRect.slot); // must not overlapped
		}

		renderData.localInstancedData.emplace_back(std::make_pair(paramTexCoordRect.slot, Bytes::Null()));
		if (paramInstanceColor.slot != paramTexCoordRect.slot) {
			renderData.localInstancedData.emplace_back(std::make_pair(paramInstanceColor.slot, Bytes::Null()));
			renderData.localInstancedData[1].second = std::move(localInstancedColorData);
		}

		renderData.localInstancedData[0].second = std::move(localInstancedData);
	}

	return verify_cast<uint32_t>(outputDrawCalls.size()) - start;
}

uint32_t TextViewComponent::GetLineCount() const {
	return verify_cast<uint32_t>(lines.size());
}

void TextViewComponent::SetPasswordChar(int ch) {
	passwordChar = ch;
}

void TextViewComponent::SetSize(Engine& engine, const Short2& s) {
	size = s;
	UpdateRenderData(engine);
}

const Short2& TextViewComponent::GetSize() const {
	return size;
}

struct LocateLineOffset {
	bool operator () (const TextViewComponent::Element& desc, int offset) {
		return desc.firstOffset < offset;
	}
};

struct LocatePosOffset {
	bool operator () (const TextViewComponent::Element::Char& desc, int offset) {
		return desc.offset < offset;
	}
};

struct LocateLine {
	bool operator () (const TextViewComponent::Element& desc, const Short2& pt) {
		return desc.yCoord < pt.y();
	}
};

struct LocatePos {
	bool operator () (const TextViewComponent::Element::Char& desc, const Short2& pt) {
		return desc.xCoord < pt.x();
	}
};

int32_t TextViewComponent::Locate(Short2& rowCol, const Short2& pt, bool isPtRowCol) const {
	if (lines.empty()) {
		rowCol = Short2(0, 0);
		return 0;
	}

	if (isPtRowCol) {
		const Element& desc = lines[rowCol.y()];
		rowCol.x() = Math::Max((int16_t)0, Math::Min((int16_t)(lines.size() - 1), pt.x()));
		rowCol.y() = Math::Max((int16_t)0, Math::Min((int16_t)(desc.allOffsets.size()), pt.y()));

		if (rowCol.y() == desc.allOffsets.size()) {
			return verify_cast<uint32_t>(text.size());
		} else {
			return desc.allOffsets[rowCol.y()].offset;
		}
	} else {
		std::vector<Element>::const_iterator p = std::lower_bound(lines.begin(), lines.end(), pt, LocateLine());
		if (p == lines.end()) {
			--p;
		}
		rowCol.x() = (int16_t)(p - lines.begin());

		std::vector<TextViewComponent::Element::Char>::const_iterator t = std::lower_bound(p->allOffsets.begin(), p->allOffsets.end(), pt, LocatePos());

		rowCol.y() = (int16_t)(t - p->allOffsets.begin());
		if (t == p->allOffsets.end()) {
			std::vector<Element>::const_iterator q = p;
			++q;
			if (q != lines.end() && !(*q).allOffsets.empty()) {
				return q->firstOffset;
			} else {
				return verify_cast<uint32_t>(text.size());
			}
		} else {
			return t->offset;
		}
	}
}

bool TextViewComponent::IsEmpty() const {
	return text.empty();
}

void TextViewComponent::SetUpdateMark() {
	Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
}