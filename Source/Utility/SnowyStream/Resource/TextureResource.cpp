#include "TextureResource.h"
#include "../Manager/RenderResourceManager.h"
#include "../../../General/Interface/IImage.h"
#include "../../../Core/System/MemoryStream.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"

#if !defined(CMAKE_PAINTSNOW) || ADD_FILTER_BPTC_BUILTIN
#include "../../../General/Driver/Filter/BPTC/ZFilterBPTC.h"
#endif

#if ADD_FILTER_ASTC_BUILTIN
#include "../../../General/Driver/Filter/ASTC/ZFilterASTC.h"
#endif

using namespace PaintsNow;

TextureResource::TextureResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), instance(nullptr), deviceMemoryUsage(0) {
	description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

void TextureResource::Attach(IRender& render, void* deviceContext) {
	BaseClass::Attach(render, deviceContext);
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	assert(queue != nullptr);
	assert(instance == nullptr);
	instance = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_TEXTURE);
}

void TextureResource::Detach(IRender& render, void* deviceContext) {
	if (instance != nullptr) {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
		render.DeleteResource(queue, instance);
		instance = nullptr;
	}

	BaseClass::Detach(render, deviceContext);
}

bool TextureResource::UnMap() {
	if (BaseClass::UnMap()) {
		ThreadPool& threadPool = resourceManager.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
			if (mapCount.load(std::memory_order_relaxed) == 0) {
				description.data.Clear();
			}
			SpinUnLock(critical);
		} 

		return true;
	} else {
		return false;
	}
}

void TextureResource::Upload(IRender& render, void* deviceContext) {
	if (Flag().load(std::memory_order_acquire) & RESOURCE_UPLOADED)
		return;
	OPTICK_EVENT();

	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	if (Flag().fetch_and(~TINY_MODIFIED) & TINY_MODIFIED) {
		assert(description.dimension.x() != 0);
		assert(description.dimension.y() != 0);
		assert(description.dimension.z() != 0);

		// Temporal code: fix addressing for cube resource
		if (description.state.format == IRender::Resource::TextureDescription::TEXTURE_2D_CUBE) {
			description.state.addressU = description.state.addressV = description.state.addressW = IRender::Resource::TextureDescription::CLAMP;
		}

		ThreadPool& threadPool = resourceManager.GetThreadPool();
		if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
			deviceMemoryUsage = description.data.GetSize();
			IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(render.MapResource(queue, instance, 0));

			if (mapCount.load(std::memory_order_relaxed) != 0) {
				desc = description;
			} else {
				desc = std::move(description);
			}

#ifdef _DEBUG
			render.SetResourceNote(instance, GetLocation());
#endif

			render.UnmapResource(queue, instance, IRender::MAP_DATA_EXCHANGE);
			SpinUnLock(critical);
			RenderResourceBase::Upload(render, deviceContext);
		}
	}
}

void TextureResource::Download(IRender& render, void* deviceContext) {
	// data.resize((size_t)dimension.x() * dimension.y() * IImage::GetPixelSize((IRender::Resource::TextureDescription::Format)state.format, (IRender::Resource::TextureDescription::Layout)state.layout));
	/*
	IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(deviceContext);
	render.RequestDownloadResource(queue, instance, this);*/
}

IRender::Resource* TextureResource::GetRenderResource() const {
	return instance;
}

TObject<IReflect>& TextureResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		// serializable
		IRender::Resource::TextureDescription::State& state = description.state;
		UShort3& dimension = description.dimension;
		Bytes& data = description.data;

		ReflectProperty(state);
		ReflectProperty(dimension);
		ReflectProperty(data);

		// runtime
		// ReflectProperty(instance)[Runtime];
	}

	return *this;
}

size_t TextureResource::ReportDeviceMemoryUsage() const {
	return deviceMemoryUsage;
}

bool TextureResource::Compress(const String& compressionType, bool refreshRuntime) {
#if !defined(CMAKE_ANDROID) && (!defined(CMAKE_PAINTSNOW) || ADD_FILTER_BPTC_BUILTIN || ADD_FILTER_ASTC_BUILTIN)
	OPTICK_EVENT();
	IFilterBase* filterFactory = nullptr;
	static ZFilterBPTC factoryBPTC;
	IRender::Resource::TextureDescription::Compress compress;

	#if !defined(_MSC_VER) || _MSC_VER > 1200
	static ZFilterASTC factoryASTC;
	#endif
	
	if (compressionType == "BPTC") { // BC7
		filterFactory = &factoryBPTC;
		compress = IRender::Resource::TextureDescription::BPTC;
	} else if (compressionType == "ASTC") {
	#if ADD_FILTER_ASTC_BUILTIN
		filterFactory = &factoryASTC;
		compress = IRender::Resource::TextureDescription::ASTC;
	#endif
	}

	if (filterFactory != nullptr) {
		const UShort3& dimension = description.dimension;
		uint32_t w = dimension.x(), h = dimension.y();
		if (w < 4 || w != h || description.data.Empty() || description.state.compress) return false;
		if (description.state.format != IRender::Resource::TextureDescription::UNSIGNED_BYTE) return false;

		MemoryStream src(sizeof(UChar4) * w * h, 128);
		if (description.state.layout == IRender::Resource::TextureDescription::RGBA) {
			memcpy(src.GetBuffer(), description.data.GetData(), description.data.GetSize());
		} else {
			// padding to RGBA8
			char* buf = (char*)src.GetBuffer();
			memset(buf, 0xFF, src.GetTotalLength());
			const uint8_t* p = description.data.GetData();
			uint32_t dim = description.state.layout;
			for (uint32_t i = 0; i < w * h; i++) {
				for (uint32_t k = 0; k < dim; k++) {
					buf[i * 4 + k] = *p++;
				}
			}
		}

		// get mip count
		uint32_t mipCount = 1;

		if (description.state.mip != IRender::Resource::TextureDescription::NOMIP) {
			description.state.mip = IRender::Resource::TextureDescription::SPECMIP;
			mipCount = Math::Log2x((uint32_t)dimension.x()) - 2;
		}

		size_t length = 0;
		for (uint32_t i = 0; i < mipCount; i++) {
			length += w * h;
			w >>= 1;
			h >>= 1;
		}

		MemoryStream target(length, 128);
		IStreamBase* filter = filterFactory->CreateFilter(target);

		w = dimension.x();
		h = dimension.y();

		for (uint32_t k = 0; k < mipCount; k++) {
			size_t len = w * h * sizeof(UChar4);
			filter->Write(src.GetBuffer(), len);

			if (k != mipCount - 1) {
				// generate mip data
				UChar4* p = reinterpret_cast<UChar4*>(src.GetBuffer());
				for (uint32_t y = 0; y < h / 2; y++) {
					for (uint32_t x = 0; x < w / 2; x++) {
						UShort4 result;
						for (uint32_t yy = 0; yy < 2; yy++) {
							for (uint32_t xx = 0; xx < 2; xx++) {
								const UChar4& v = p[(y * 2 + yy) * w + (x * 2 + xx)];
								for (uint32_t m = 0; m < 4; m++) {
									result[m] += v[m];
								}
							}
						}

						p[y * w / 2 + x] = UChar4(result[0] >> 2, result[1] >> 2, result[2] >> 2, result[3] >> 2);
					}
				}
			}

			w >>= 1;
			h >>= 1;
		}

		if (refreshRuntime) {
			// TODO: conflicts with mapped resource
			assert(mapCount.load(std::memory_order_relaxed) == 0);
			ThreadPool& threadPool = resourceManager.GetThreadPool();
			if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
				description.data.Assign((uint8_t*)target.GetBuffer(), verify_cast<uint32_t>(target.GetTotalLength()));
				description.state.compress = compress;
				description.state.layout = IRender::Resource::TextureDescription::RGBA;

				// TODO: InvokeUpload
				SpinUnLock(critical);
			}
		} else {
			// Write to file
			TShared<TextureResource> compressedTextureResource = TShared<TextureResource>::From(new TextureResource(resourceManager, GetLocation() + "/" + compressionType));
			compressedTextureResource->description.data.Assign((uint8_t*)target.GetBuffer(), verify_cast<uint32_t>(target.GetTotalLength()));
			compressedTextureResource->description.dimension = description.dimension;
			compressedTextureResource->description.state = description.state;
			compressedTextureResource->description.state.compress = compress;
			compressedTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
			resourceManager.GetUniformResourceManager().SaveResource(compressedTextureResource());
		}

		filter->Destroy();
		return true;
	} else {
		return false;
	}
#else
	return false;
#endif
}

bool TextureResource::LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) {
	IImage& imageBase = interfaces.image;
	IImage::Image* image = imageBase.Create(1, 1, IRender::Resource::TextureDescription::RGB, IRender::Resource::TextureDescription::Format::UNSIGNED_BYTE);
	if (image == nullptr) return false;
	bool success = imageBase.Load(image, streamBase, length);
	IRender::Resource::TextureDescription::Layout layout = imageBase.GetLayoutType(image);
	IRender::Resource::TextureDescription::Format dataType = imageBase.GetDataType(image);

	ThreadPool& threadPool = resourceManager.GetThreadPool();
	if (threadPool.PollExchange(threadPool.GetCurrentThreadIndex(), critical, 1u) == 0u) {
		description.state.layout = layout;
		description.state.format = dataType;
		description.dimension.x() = verify_cast<uint16_t>(imageBase.GetWidth(image));
		description.dimension.y() = verify_cast<uint16_t>(imageBase.GetHeight(image));
		description.dimension.z() = 1;

		void* buffer = imageBase.GetBuffer(image);
		description.data.Assign(reinterpret_cast<uint8_t*>(buffer), (size_t)description.dimension.x() * description.dimension.y() * IImage::GetPixelBitDepth(dataType, layout) / 8);
		SpinUnLock(critical);
	}

	// copy info
	imageBase.Delete(image);
	return success;
}


IStreamBase* TextureResource::OpenArchive(IArchive& archive, const String& extension, bool write, uint64_t& length) {
	IStreamBase* compressed = nullptr;

	// find alternative
	assert(extension == "TextureResource");
	String location = GetLocation();
	assert(!location.empty());
	if (location[location.size() - 1] == '$') { // raw resource required
		return archive.Open(location.substr(0, location.size() - 1) + "." + extension + resourceManager.GetLocationPostfix(), write, length);
	} else {
#if defined(CMAKE_ANDROID)
		// Try astc
		compressed = archive.Open(location + "/ASTC" + "." + extension + resourceManager.GetLocationPostfix(), write, length);
#else
		compressed = archive.Open(location + "/BPTC" + "." + extension + resourceManager.GetLocationPostfix(), write, length);
#endif

		if (compressed != nullptr) {
			Flag().fetch_or(RESOURCE_COMPRESSED, std::memory_order_release);
			return compressed;
		} else {
			Flag().fetch_and(~RESOURCE_COMPRESSED, std::memory_order_release);
			return BaseClass::OpenArchive(archive, extension, write, length);
		}
	}
}

TShared<TextureResource> TextureResource::MapRawTexture() {
	if (rawTexture() != nullptr) {
		rawTexture->Map();
		return rawTexture;
	} else if (Flag().load(std::memory_order_acquire) & RESOURCE_COMPRESSED) {
		TShared<ResourceBase> raw = resourceManager.GetUniformResourceManager().CreateResource(GetLocation() + '$', "TextureResource", true, RESOURCE_MAPPED);

		if (raw() != nullptr) {
			TShared<TextureResource> tex = raw->QueryInterface(UniqueType<TextureResource>());
			assert(tex() != nullptr);

			TSpinLockGuard<uint32_t> guard(critical);
			if (rawTexture() != nullptr) {
				rawTexture = tex;
			}

			return tex;
		} else {
			return nullptr;
		}
	} else {
		Map();
		return this;
	}
}

