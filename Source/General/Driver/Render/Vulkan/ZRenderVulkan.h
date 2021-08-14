// ZRenderVulkan.h -- Vulkan Render API provider
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#pragma once
#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IRender.h"
#include "../../../../Core/Interface/IThread.h"
#include "../../../../Core/Interface/IReflect.h"
#include "../../../../Core/Template/TQueue.h"

typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
struct VkAllocationCallbacks;
struct GLFWwindow;

namespace PaintsNow {
	class ZRenderVulkan final : public IRender {
	public:
		ZRenderVulkan(GLFWwindow* window);
		~ZRenderVulkan() override;

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

	protected:
		GLFWwindow* window;
		VkInstance instance;
		size_t surface;
		std::atomic<uint32_t> frameIndex;
		std::vector<VkPhysicalDevice> gpus;

		#ifdef _DEBUG
		size_t debugCallback;
		#endif
	};
}
