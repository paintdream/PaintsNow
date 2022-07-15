// RenderResourceBase.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#pragma once
#include "../ResourceBase.h"
#include "../../../General/Interface/IRender.h"
#include "../../../General/Misc/PassBase.h"

namespace PaintsNow {
	class IDataUpdater {
	public:
		virtual uint32_t Update(IRender& render, IRender::Queue* queue) = 0;
	};

	template <class T>
	struct MapFormat {};

	template <>
	struct MapFormat<float> {
		enum { format = IRender::Resource::BufferDescription::FLOAT };
	};

	template <>
	struct MapFormat<uint8_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_BYTE };
	};

	template <>
	struct MapFormat<uint16_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_SHORT };
	};

	template <>
	struct MapFormat<uint32_t> {
		enum { format = IRender::Resource::BufferDescription::UNSIGNED_INT };
	};

	class RenderResourceManager;
	class RenderResourceBase : public TReflected<RenderResourceBase, DeviceResourceBase<IRender> > {
	public:
		static inline void ClearBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer) {
			if (buffer != nullptr) {
				render.DeleteResource(queue, buffer);
				buffer = nullptr;
			}
		}

		template <class T>
		inline uint32_t UpdateBuffer(IRender& render, IRender::Queue* queue, IRender::Resource*& buffer, std::vector<T>& data, IRender::Resource::BufferDescription::Usage usage, const char* note, uint32_t groupSize = 1) {
			uint32_t size = verify_cast<uint32_t>(data.size());
			if (!data.empty()) {
				if (buffer == nullptr) {
					buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
				}

				IRender::Resource::BufferDescription& description = *static_cast<IRender::Resource::BufferDescription*>(render.MapResource(queue, buffer, 0));
				description.data.Resize(verify_cast<uint32_t>(data.size() * sizeof(T)));
				description.state.usage = usage;
#if defined(_MSC_VER) && _MSC_VER <= 1200
				description.state.component = verify_cast<uint8_t>(sizeof(T) / sizeof(T::type) * groupSize);
				description.state.format = MapFormat<T::type>::format;
#else
				description.state.component = verify_cast<uint8_t>(sizeof(T) / sizeof(typename T::type) * groupSize);
				description.state.format = MapFormat<typename T::type>::format;
#endif
				description.state.stride = verify_cast<uint16_t>(sizeof(T) * groupSize);
				memcpy(description.data.GetData(), &data[0], data.size() * sizeof(T));
#ifdef _DEBUG
				render.SetResourceNote(buffer, GetLocation() + "@" + note);
#endif
				render.UnmapResource(queue, buffer, IRender::MAP_DATA_EXCHANGE);
			}

			return size;
		}

		RenderResourceBase(ResourceManager& manager, const String& uniqueID);
		~RenderResourceBase() override;
		bool Complete(size_t version) override;
		void Upload(IRender& device, void* deviceContext) override;
		void Refresh(IRender& device, void* deviceContext) override;
		void Attach(IRender& device, void* deviceContext) override;
		void Detach(IRender& device, void* deviceContext) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderResourceManager& GetRenderResourceManager();
		void OnResourceUploaded(IRender& device, IRender::Queue* queue, IRender::Resource* event, bool success);
		struct ResourceUploadCallback;

	protected:
		std::atomic<size_t> runtimeVersion; // for resource updating synchronization
	};
}

