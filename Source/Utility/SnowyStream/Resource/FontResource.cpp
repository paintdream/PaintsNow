#include "FontResource.h"
#include "../../SnowyStream/SnowyStream.h"
#include "../../SnowyStream/Manager/RenderResourceManager.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../Core/System/MemoryStream.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

FontResource::FontResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), font(nullptr), dim(512), weight(0) {}
FontResource::~FontResource() {}

bool FontResource::LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) {
	rawFontData.resize(length);
	return streamBase.ReadBlock(const_cast<char*>(rawFontData.data()), length);
}

void FontResource::Refresh(IFontBase& fontBase, void* deviceContext) {

}

void FontResource::Attach(IFontBase& fontBase, void* deviceContext) {
}

void FontResource::Detach(IFontBase& fontBase, void* deviceContext) {
	SnowyStream* snowyStream = reinterpret_cast<SnowyStream*>(deviceContext);
	IRender& render = snowyStream->GetInterfaces().render;
	IRender::Queue* queue = snowyStream->GetRenderResourceManager()->GetResourceQueue();

	for (std::map<uint32_t, Slice>::iterator it = sliceMap.begin(); it != sliceMap.end(); ++it) {
		it->second.Uninitialize(render, queue, resourceManager);
	}

	sliceMap.clear();

	if (font != nullptr) {
		fontBase.Close(font);
		font = nullptr;
	}
}

void FontResource::Upload(IFontBase& fontBase, void* deviceContext) {
	if (font == nullptr && !rawFontData.empty()) {
		OPTICK_EVENT();
		// load font resource from memory
		MemoryStream ms(rawFontData.size());
		size_t len = rawFontData.size();
		if (ms.WriteBlock(rawFontData.data(), len)) {
			ms.Seek(IStreamBase::BEGIN, 0);
			font = fontBase.Load(ms, len);
		}

		if (font == nullptr) {
			// report error
			resourceManager.Report(String("Unable to load font resource: ") + uniqueLocation);
		}
	}
}

void FontResource::Download(IFontBase& fontBase, void* deviceContext) {
}

TObject<IReflect>& FontResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// serializable
		ReflectProperty(rawFontData);
		ReflectProperty(dim);
		ReflectProperty(weight);

		// runtime
		ReflectProperty(sliceMap)[Runtime];
		ReflectProperty(font)[Runtime];
	}

	return *this;
}

const FontResource::Char& FontResource::Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size) {
	size = size == 0 ? 14 : size;

	TSpinLockGuard<uint32_t> guard(critical);
	Slice& slice = sliceMap[size];
	guard.UnLock();

	if (slice.fontSize == 0) { // not initialized?
		slice.fontSize = size;
		slice.font = font;
		slice.dim = dim;
	}

	const FontResource::Char& ret = slice.Get(render, queue, fontBase, ch);
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(slice.critical);

	if (lock.load(std::memory_order_relaxed) == 2u) {
		// need update
		Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
	}

	return ret;
}

FontResource::Slice::Slice(uint16_t fs, uint16_t d) : font(nullptr), fontSize(fs), critical(0), dim(d), lastRect(Short2(0, 0), Short2(0, 0)) {
}

void FontResource::Slice::Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	for (size_t i = 0; i < cacheTextures.size(); i++) {
		render.DeleteResource(queue, cacheTextures[i]());
	}

	cacheTextures.clear();
}

Short2Pair FontResource::Slice::AllocRect(IRender& render, IRender::Queue* queue, const Short2& size) {
	assert(size.x() <= dim);
	assert(size.y() <= dim);

	if (lastRect.second.y() + size.y() > dim || cacheTextures.empty()) {
		IRender::Resource* texture = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_TEXTURE);
		cacheTextures.push_back(texture);
		lastRect.first.y() = 0;
		lastRect.second.y() = size.y();
	}

	if (lastRect.second.x() + size.x() > dim) {
		// new line
		lastRect.second.x() = 0;
		lastRect.first.y() = lastRect.second.y() + 1;
	}

	uint16_t height = Math::Max((int16_t)(lastRect.second.y() - lastRect.first.y()), size.y());
	Short2Pair w;
	w.first.x() = lastRect.second.x();
	w.second.x() = w.first.x() + size.x();
	w.second.y() = lastRect.first.y() + height;
	w.first.y() = w.second.y() - size.y();

	lastRect.first.x() = w.first.x();
	lastRect.second.x() = w.second.x() + 1;
	lastRect.second.y() = Math::Max(w.second.y(), lastRect.second.y());

	assert(!cacheTextures.empty());
	cacheTextures.back().Tag(1); // set dirty

	return w;
}

uint32_t FontResource::Slice::UpdateFontTexture(IRender& render, IRender::Queue* queue) {
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(critical);
	uint32_t count = 0;

	if (SpinLock(lock) == 2u) {
		for (size_t i = 0; i < cacheTextures.size(); i++) {
			TTagged<IRender::Resource*, 2>& ptr = cacheTextures[i];

			if (ptr.Tag()) {
				IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(render.MapResource(queue, ptr(), 0));
				desc.data = buffer;
				desc.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
				desc.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
				desc.state.layout = IRender::Resource::TextureDescription::R;
				desc.dimension.x() = dim;
				desc.dimension.y() = dim;
				desc.dimension.z() = 0;

				render.UnmapResource(queue, ptr(), IRender::MAP_DATA_EXCHANGE);
				ptr.Tag(0);
				count++;
			}
		}
	}
	
	SpinUnLock(lock);
	return count;
}

Short2 FontResource::Slice::GetTextureSize() const {
	return Short2(dim, dim);
}

uint16_t FontResource::GetFontTextureSize() const {
	return dim;
}

uint32_t FontResource::Update(IRender& render, IRender::Queue* queue) {
	uint32_t count = 0;
	if (Flag().load(std::memory_order_acquire) & TINY_MODIFIED) {
		for (std::map<uint32_t, Slice>::iterator it = sliceMap.begin(); it != sliceMap.end(); ++it) {
			std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(it->second.critical);

			if (lock.load(std::memory_order_relaxed) == 2u) {
				count += it->second.UpdateFontTexture(render, queue);
			}
		}

		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}

	return count;
}

const FontResource::Char& FontResource::Slice::Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch) {
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(critical);
	TSpinLockGuard<uint32_t> guard(lock);
	FontMap::iterator p = cache.find(ch);
	if (p != cache.end()) {
		return (*p).second;
	} else {
		assert(font != nullptr);
		if (buffer.Empty()) {
			buffer.Resize(dim * dim * sizeof(uint8_t), 0);
		}

		Char c;
		String data;
		c.info = fontBase.RenderTexture(font, data, ch, fontSize, 0);
		c.rect = AllocRect(render, queue, Short2(c.info.width, c.info.height));
		c.textureResource = cacheTextures.back()();
		guard.UnLock();

		Short2Pair& r = c.rect;
		assert(r.second.x() <= dim && r.second.y() <= dim);
		uint8_t* target = buffer.GetData();

		const uint8_t* p = (const uint8_t*)data.data();
		for (int j = r.first.y(); j < r.second.y(); j++) {
			for (int i = r.first.x(); i < r.second.x(); i++) {
				target[j * dim + i] = *p++;
			}
		}

		TSpinLockGuard<uint32_t, 1u, 2u> innerGuard(lock);
		Char& ret = cache[ch] = c;
		return ret;
	}
}

