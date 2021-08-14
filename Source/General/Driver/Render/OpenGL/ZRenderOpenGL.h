// ZRenderOpenGL.h -- OpenGL Render API provider
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IRender.h"
#include "../../../../Core/Interface/IThread.h"
#include "../../../../Core/Interface/IReflect.h"
#include "../../../../Core/Template/TQueue.h"

namespace PaintsNow {
	class ZRenderOpenGL final : public IRender {
	public:
		ZRenderOpenGL();
		~ZRenderOpenGL() override;

		std::vector<String> EnumerateDevices() override;
		Device* CreateDevice(const String& description) override;
		Int2 GetDeviceResolution(Device* device) override;
		void SetDeviceResolution(Device* device, const Int2& resolution) override;
		bool NextDeviceFrame(Device* device) override;
		void DeleteDevice(Device* device) override;

		void SubmitQueues(Queue** queues, uint32_t count, SubmitOption option) override;
		size_t GetProfile(Device* device, const String& feature) override;

		// Queue
		Queue* CreateQueue(Device* device, uint32_t flag) override;
		Device* GetQueueDevice(Queue* queue) override;
		void DeleteQueue(Queue* queue) override;
		void FlushQueue(Queue* queue) override;

		// Resource
		Resource* CreateResource(Device* device, Resource::Type resourceType) override;
		void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		void SetupBarrier(Queue* queue, Barrier* barrier) override;
		const void* GetResourceDeviceHandle(Resource* resource) override;
		void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		void CompleteDownloadResource(Queue* queue, Resource* resource) override;
		void ExecuteResource(Queue* queue, Resource* resource) override;
		void DeleteResource(Queue* queue, Resource* resource) override;
		void SetResourceNotation(Resource* lhs, const String& note) override;
		const String& GetShaderVersion() const;

	protected:
		void ClearDeletedQueues();

	protected:
		std::atomic<Queue*> deletedQueueHead;
		uint32_t majorVersion;
		uint32_t minorVersion;
		String shaderVersion;
	};
}
