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
		bool PreDeviceFrame(Device* device) override;
		void PostDeviceFrame(Device* device) override;
		void DeleteDevice(Device* device) override;

		void SubmitQueues(Queue** queues, uint32_t count, SubmitOption option) override;
		size_t GetProfile(Device* device, const String& feature) override;

		// Queue
		Queue* CreateQueue(Device* device, uint32_t flag) override;
		Device* GetQueueDevice(Queue* queue) override;
		void DeleteQueue(Queue* queue) override;
		void FlushQueue(Queue* queue) override;
		bool IsQueueEmpty(Queue* queue) override;

		// Resource
		Resource* CreateResource(Device* device, Resource::Type resourceType, Queue* optionalHostQueue) override;
		const void* GetResourceDeviceHandle(IRender::Resource* resource) override;
		void DeleteResource(Queue* queue, Resource* resource) override;
		Resource::Description* MapResource(Queue* queue, Resource* resource, uint32_t mapFlags) override;
		void UnmapResource(Queue* queue, Resource* resource, uint32_t mapFlags) override;
		void SetupBarrier(Queue* queue, Barrier* barrier) override;
		void ExecuteResource(Queue* queue, Resource* resource) override;
		void SetResourceNote(Resource* lhs, const String& note) override;

		void BuildSwapChain(IRender::Device* dev);

	protected:
		GLFWwindow* window;
		VkInstance instance;
		uint64_t surface;
		std::atomic<uint32_t> frameIndex;
		std::vector<VkPhysicalDevice> gpus;

		#ifdef _DEBUG
		uint64_t debugCallback;
		#endif
	};
}
