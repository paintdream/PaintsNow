// BatchComponent.h
// PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "../../Entity.h"
#include "../../Component.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	// Manages Vertex/Uniform/Instance buffers statically
	class RenderableComponent;
	class BatchComponent : public TAllocatedTiny<BatchComponent, Component>, public IDataUpdater {
	public:
		BatchComponent(IRender::Resource::BufferDescription::Usage usage);
		~BatchComponent() override;

		template <class T>
		IRender::Resource::DrawCallDescription::BufferRange Allocate(const T& data) {
			return Allocate(&data, sizeof(data));
		}

		template <class T>
		IRender::Resource::DrawCallDescription::BufferRange AllocateSafe(const T& data) {
			return AllocateSafe(&data, sizeof(data));
		}

		IRender::Resource::DrawCallDescription::BufferRange Allocate(const void* data, uint32_t size);
		IRender::Resource::DrawCallDescription::BufferRange AllocateSafe(const void* data, uint32_t size);
		uint32_t Update(IRender& render, IRender::Queue* queue) override;
		void InstanceInitialize(Engine& engine);
		void InstanceUninitialize(Engine& engine);

		Bytes& GetCurrentData();

		IRender::Resource::BufferDescription::Usage GetBufferUsage() const;

	private:
		std::vector<uint8_t> currentData;
		IRender::Resource* buffer;
		std::atomic<uint32_t> referenceCount;
		std::atomic<uint32_t> critical;
		IRender::Resource::BufferDescription::Usage bufferUsage;
	};
}

