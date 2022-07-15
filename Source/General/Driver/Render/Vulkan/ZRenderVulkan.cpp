#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "../../../../Core/Template/TPool.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include "../OpenGL/GLSLShaderGenerator.h"
#include "../../../../General/Misc/PreConstExpr.h"
#include "SPIRVCompiler.h"
#include "ZRenderVulkan.h"
// #include <glslang/Public/ShaderLang.h>
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>
// #include <windows.h>

#define GLFW_INCLUDE_VULKAN
#include "../../Frame/GLFW/Core/include/GLFW/glfw3.h"

using namespace PaintsNow;

const int MIN_IMAGE_COUNT = 2;
const int MAX_DESCRIPTOR_SIZE = 1000;

static void Verify(const char* message, VkResult res) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "Unable to %s (debug).\n", message);
		assert(false);
	}
}

class QueueImplVulkan;
class DeviceImplVulkan;
class_aligned(8) ResourceBaseImplVulkan : public IRender::Queue, public Void /* IRender::Resource */{ // here is a trick for deriving queue
public:
	typedef void (ResourceBaseImplVulkan::*Action)(QueueImplVulkan& queue);
	typedef const void* (ResourceBaseImplVulkan::*GetRawHandle)() const;
	typedef IRender::Resource::Description ResourceBaseImplVulkan::* DescriptionAddress;
	ResourceBaseImplVulkan() {
		downloadDescription.store(nullptr, std::memory_order_relaxed);
	}

	struct DispatchTable {
		IRender::Resource::Type type;
		DescriptionAddress descriptionAddress;
		inline Action GetAction(uint32_t index) const {
			return *(&actionExecute + index);
		}

		inline uint32_t GetActionCount() const {
			return 4;
		}

		Action actionExecute;
		Action actionUpload;
		Action actionDownload;
		Action actionDelete;
		Action actionSynchronizeDownload;
		GetRawHandle getHandle;
	};

	DispatchTable* dispatchTable;
	std::atomic<IRender::Resource::Description*> downloadDescription;

#ifdef _DEBUG
	String note;
#endif
};

struct FrameData {
	std::vector<QueueImplVulkan*> activeQueues;
	VkImage backBufferImage;
	VkImageView backBufferView;
	VkFence fence;
	VkCommandBuffer mainCommandBuffer;
};

struct SemaphoreData {
	VkSemaphore acquireSemaphore;
	VkSemaphore releaseSemaphore;
};

struct DrawSignatureItem {
	uint8_t format : 3;
	uint8_t component : 5;
};

class DeviceImplVulkan final : public IRender::Device {
public:
	enum { MAX_DESCRIPTOR_COUNT = 1024 };
	DeviceImplVulkan(IRender& r, VkAllocationCallbacks* alloc, VkPhysicalDevice phy, VkDevice dev, uint32_t family, VkQueue q) : render(r), allocator(alloc), physicalDevice(phy), device(dev), resolution(0, 0), queueFamily(family), queue(q), swapChain(VK_NULL_HANDLE), currentFrameIndex(0), currentSemaphoreIndex(0), surfaceFormat(VK_FORMAT_B8G8R8A8_UNORM), swapChainRebuild(false), swapChainRebuilding(false), repeatQueueStarvationCount(0), queueSummaryFlag(0) {
		for (size_t k = 0; k < sizeof(descriptorAllocators) / sizeof(descriptorAllocators[0]); k++) {
			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.maxSets = MAX_DESCRIPTOR_COUNT;
			poolInfo.poolSizeCount = 1;
			VkDescriptorPoolSize size;
			size.descriptorCount = MAX_DESCRIPTOR_COUNT;
			size.type = (VkDescriptorType)k;
			poolInfo.pPoolSizes = &size;
			Verify("create descriptor pool", vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorAllocators[k]));
		}

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProps);

		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = queueFamily;
		vkCreateCommandPool(device, &info, allocator, &mainCommandPool);
		queueCritical.store(0, std::memory_order_relaxed);
		descriptorCritical.store(0, std::memory_order_release);
	}

	uint32_t GetMemoryType(VkMemoryPropertyFlags properties, uint32_t bits) {
		for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++)
			if ((memoryProps.memoryTypes[i].propertyFlags & properties) == properties && bits & (1 << i))
				return i;

		return ~(uint32_t)0;
	}

	void DestroyAllFrames(VkAllocationCallbacks* allocator, bool cleanupQueue = true) {
		for (size_t i = 0; i < frames.size(); i++) {
			FrameData& frameData = frames[i];
			if (cleanupQueue) {
				CleanupFrameQueue(frameData);
			}

			// vkDestroyImage(device, frameData.backBufferImage, allocator); // backBufferImage is auto-released in destroying swap chain
			vkFreeCommandBuffers(device, mainCommandPool, 1, &frameData.mainCommandBuffer);
			vkDestroyImageView(device, frameData.backBufferView, allocator);
			vkDestroyFence(device, frameData.fence, allocator);
		}

		while (DeleteFrameQueue()) {}
		frames.clear();

		for (size_t j = 0; j < semaphores.size(); j++) {
			SemaphoreData& semaphoreData = semaphores[j];
			vkDestroySemaphore(device, semaphoreData.acquireSemaphore, allocator);
			vkDestroySemaphore(device, semaphoreData.releaseSemaphore, allocator);
		}

		semaphores.clear();
	}

	~DeviceImplVulkan() {
		vkDeviceWaitIdle(device);
		DestroyAllFrames(allocator);
		vkDestroyCommandPool(device, mainCommandPool, allocator);
		vkDestroySwapchainKHR(device, swapChain, allocator);

		for (size_t k = 0; k < sizeof(descriptorAllocators) / sizeof(descriptorAllocators[0]); k++) {
			vkDestroyDescriptorPool(device, descriptorAllocators[k], allocator);
		}

		vkDestroyDevice(device, allocator);
	}

	void CleanupFrameQueue(FrameData& lastFrame);
	bool DeleteFrameQueue();

	FrameData& GetCurrentFrame() {
		return frames[currentFrameIndex];
	}

	SemaphoreData& GetCurrentSemaphore() {
		return semaphores[currentSemaphoreIndex];
	}

	bool PreFrame() {
		OPTICK_EVENT();
		frameCommandBuffers.clear();
		repeatQueueStarvationCount = 0;
		queueSummaryFlag = 0;

		if (swapChainRebuild) {
			// rebuild swap chain
			static_cast<ZRenderVulkan&>(render).BuildSwapChain(this);
			swapChainRebuilding = true;
			return false;
		}

		return true;
	}

	void PostFrame() {
		OPTICK_EVENT();

		swapChainRebuilding = false;
		// repeat queue starvations also lead to a skipped frame in case of imcomplete pipeline settings!
		if (repeatQueueStarvationCount != 0) {
			return;
		} else {
			FrameData& frame = GetCurrentFrame();
			SemaphoreData& semaphore = GetCurrentSemaphore();

			OPTICK_PUSH("Acquire Next Image");
			VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, semaphore.acquireSemaphore, VK_NULL_HANDLE, &currentFrameIndex);
			OPTICK_POP();

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
				// TODO: rebuild swap chain
				swapChainRebuild = true;
				return;
			}

			Verify("wait frame fence", vkWaitForFences(device, 1, &frame.fence, VK_TRUE, UINT64_MAX));
		}

		FrameData& frame = GetCurrentFrame();
		SemaphoreData& semaphore = GetCurrentSemaphore();
		
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = {};

		frameCommandBuffers.emplace_back(frame.mainCommandBuffer);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphore.acquireSemaphore;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = verify_cast<uint32_t>(frameCommandBuffers.size());
		submitInfo.pCommandBuffers = frameCommandBuffers.empty() ? nullptr : &frameCommandBuffers[0];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphore.releaseSemaphore;

        Verify("reset frame fence", vkResetFences(device, 1, &frame.fence));
		Verify("queue submit", vkQueueSubmit(queue, 1, &submitInfo, frame.fence));

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &semaphore.releaseSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &currentFrameIndex;
		VkResult err = vkQueuePresentKHR(queue, &presentInfo);

		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			swapChainRebuild = true;
			return;
		}

		do {
			TSpinLockGuard<size_t> guard(queueCritical);
			deletedQueues.Push(nullptr);
		} while (false);

		// rotate frames
		CleanupFrameQueue(GetCurrentFrame());
		DeleteFrameQueue();
		currentSemaphoreIndex = (currentSemaphoreIndex + 1) % semaphores.size();
	}

	IRender& render;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkAllocationCallbacks* allocator;
	VkQueue queue;
	Int2 resolution;
	VkSwapchainKHR swapChain;

	VkPhysicalDeviceMemoryProperties memoryProps;
	VkDescriptorPool descriptorAllocators[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1];
	uint32_t queueFamily;
	uint32_t currentFrameIndex;
	uint32_t currentSemaphoreIndex;
	VkFormat surfaceFormat;
	bool swapChainRebuild;
	bool swapChainRebuilding;
	uint16_t repeatQueueStarvationCount;
	uint32_t queueSummaryFlag;
	std::vector<FrameData> frames;
	std::vector<SemaphoreData> semaphores;
	std::vector<VkCommandBuffer> frameCommandBuffers;
	TQueueList<QueueImplVulkan*> deletedQueues;
	VkCommandPool mainCommandPool;

	alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> queueCritical;
	alignas(CPU_CACHELINE_SIZE) std::atomic<size_t> descriptorCritical;
};

template <class T, IRender::Resource::Type type, class D>
class ResourceBaseVulkanDesc : public ResourceBaseImplVulkan {
public:
	typedef ResourceBaseVulkanDesc<T, type, D> Base;
	static /* constexpr */ ResourceBaseImplVulkan::DispatchTable* GetDispatchTable() {
		static /* constexpr */ ResourceBaseImplVulkan::DispatchTable dispatchTable = {
			type,
			reinterpret_cast<DescriptionAddress>(&D::description),
			reinterpret_cast<Action>(&D::Execute),
			reinterpret_cast<Action>(&D::Upload),
			reinterpret_cast<Action>(&D::Download),
			reinterpret_cast<Action>(&D::Delete),
			reinterpret_cast<Action>(&D::SynchronizeDownload),
			reinterpret_cast<GetRawHandle>(&D::GetHandle)
		};

		return &dispatchTable;
	}

	ResourceBaseVulkanDesc() {
		dispatchTable = GetDispatchTable();
	}

	void Execute(QueueImplVulkan& queue) {}
	void Upload(QueueImplVulkan& queue) {}
	void Download(QueueImplVulkan& queue) {}
	void Delete(QueueImplVulkan& queue) {}
	void SynchronizeDownload(QueueImplVulkan& queue) {}
	const void* GetHandle() const { return nullptr; }

	T description;
};

class QueueImplVulkan final : public ResourceBaseVulkanDesc<IRender::Resource::Description, IRender::Resource::RESOURCE_UNKNOWN, QueueImplVulkan> {
public:
	enum { MAX_POOLED_BUFFER_COUNT = 4 };

	struct_aligned(64) BufferNode {
		BufferNode() : flag(0), commandCount(0), hostQueue(nullptr), buffer(VK_NULL_HANDLE) {}

		uint32_t flag;
		uint32_t commandCount;
		QueueImplVulkan* hostQueue;
		VkCommandBuffer buffer;

		struct Descriptor {
			Descriptor(VkDescriptorPool p, VkDescriptorSet s) : pool(p), descriptor(s) {}
			VkDescriptorPool pool;
			VkDescriptorSet descriptor;
		};

		std::vector<VkPipelineLayout> deletedPipelineLayouts;
		std::vector<VkDescriptorSetLayout> deletedDescriptorSetLayouts;
		std::vector<VkPipeline> deletedPipelines;
		std::vector<VkShaderModule> deletedShaderModules;
		std::vector<Descriptor> deletedDescriptors;
		std::vector<VkRenderPass> deletedRenderPasses;
		std::vector<VkFramebuffer> deletedFramebuffers;
		std::vector<VkBuffer> deletedBuffers;
		std::vector<VkSampler> deletedSamplers;
		std::vector<VkImageView> deletedImageViews;
		std::vector<VkImage> deletedImages;
		std::vector<VkDeviceMemory> deletedMemories;
		std::vector<VkEvent> deletedEvents;
	};

	QueueImplVulkan(DeviceImplVulkan* dev, uint32_t f) : device(dev), renderTargetResource(nullptr), flag(f) {
		critical.store(0u, std::memory_order_relaxed);

		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		info.queueFamilyIndex = device->queueFamily;
		vkCreateCommandPool(device->device, &info, device->allocator, &commandPool);

		// make frame count delay
		assert(!dev->frames.empty());
		for (size_t i = 0; i < dev->frames.size(); i++) {
			FrameBarrier();
		}

		currentCommandBuffer = Allocate();
		BeginCurrentCommandBuffer();
	}

	void Execute(QueueImplVulkan& queue) {
		while (!preparedCommandBuffers.Empty()) {
			BufferNode node = std::move(preparedCommandBuffers.Top());
			queue.preparedCommandBuffers.Push(std::move(node));
			preparedCommandBuffers.Pop();
		}
	}

	inline void DoLock() {
		if (flag & IRender::QUEUE_MULTITHREAD) {
			SpinLock(critical);
		}
	}

	inline void UnLock() {
		if (flag & IRender::QUEUE_MULTITHREAD) {
			SpinUnLock(critical);
		}
	}

	struct LockGuard {
		LockGuard(QueueImplVulkan& q) : queue(q) {
			queue.DoLock();
		}

		~LockGuard() { queue.UnLock(); }

		QueueImplVulkan& queue;
	};

	~QueueImplVulkan() {
		do {
			FlushCommandBuffer();
			while (!preparedCommandBuffers.Empty()) {
				recycledCommandBuffers.Push(std::move(preparedCommandBuffers.Top()));
				preparedCommandBuffers.Pop();
			}

			// Clear all at once!
		} while (ClearFrameData<true>() || !recycledEvents.Empty() && !newEvents.Empty());

		EndCurrentCommandBuffer();
		recycledCommandBuffers.Push(std::move(currentCommandBuffer));
		ClearFrameData<true>();

		assert(recycledCommandBuffers.Empty());
		FinalizeCommandBuffers();
		vkDestroyCommandPool(device->device, commandPool, device->allocator);
	}

	template <bool finalize>
	bool DispatchEvents();
	template <bool finalize>
	bool DispatchEventsEx(TQueueList<ResourceBaseImplVulkan*>& events, TQueueList<ResourceBaseImplVulkan*>& nextEvents);

	void FinalizeCommandBuffers() {
		while (!finalizingCommandBuffers.Empty()) {
			Deallocate(finalizingCommandBuffers.Top());
			finalizingCommandBuffers.Pop();
		}
	}

	template <bool finalize>
	bool ClearFrameData() {
		bool next = DispatchEvents<finalize>();

		while (!recycledCommandBuffers.Empty()) {
			BufferNode bufferNode = std::move(recycledCommandBuffers.Top());
			recycledCommandBuffers.Pop();

			if (bufferNode.buffer != nullptr) {
				finalizingCommandBuffers.Push(std::move(bufferNode));
			} else {
				next = true;
				break;
			}
		}

		return next;
	}

	BufferNode Allocate() {
		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;// flag& IRender::QUEUE_SECONDARY ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = commandPool;
		info.pNext = nullptr;

		VkCommandBuffer ret = nullptr;
		if (!(flag & IRender::QUEUE_MULTITHREAD)) {
			TSpinLockGuard<uint32_t> guard(critical);
			vkAllocateCommandBuffers(device->device, &info, &ret);
		} else {
			vkAllocateCommandBuffers(device->device, &info, &ret);
		}

		BufferNode node;
		node.flag = 0;
		node.buffer = ret;
		node.hostQueue = this;
		return node;
	}

	void Deallocate(BufferNode& node) {
		// free resources
		VkDevice dev = device->device;
		VkAllocationCallbacks* allocator = device->allocator;

		for (VkPipelineLayout pipelineLayout : node.deletedPipelineLayouts) {
			vkDestroyPipelineLayout(dev, pipelineLayout, allocator);
		}

		for (VkDescriptorSetLayout descriptorSetLayout : node.deletedDescriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(dev, descriptorSetLayout, allocator);
		}

		for (VkPipeline pipeline : node.deletedPipelines) {
			vkDestroyPipeline(dev, pipeline, allocator);
		}

		for (VkShaderModule shaderModule : node.deletedShaderModules) {
			vkDestroyShaderModule(dev, shaderModule, allocator);
		}

		do {
			TSpinLockGuard<size_t> guard(device->descriptorCritical);
			for (BufferNode::Descriptor& descriptor : node.deletedDescriptors) {
				vkFreeDescriptorSets(dev, descriptor.pool, 1, &descriptor.descriptor);
			}
		} while (false);

		for (VkRenderPass renderPass : node.deletedRenderPasses) {
			vkDestroyRenderPass(dev, renderPass, allocator);
		}

		for (VkFramebuffer framebuffer : node.deletedFramebuffers) {
			vkDestroyFramebuffer(dev, framebuffer, allocator);
		}

		for (VkBuffer buffer : node.deletedBuffers) {
			vkDestroyBuffer(dev, buffer, allocator);
		}

		for (VkSampler sampler : node.deletedSamplers) {
			vkDestroySampler(dev, sampler, allocator);
		}

		for (VkImageView imageView : node.deletedImageViews) {
			vkDestroyImageView(dev, imageView, allocator);
		}

		for (VkImage image : node.deletedImages) {
			vkDestroyImage(dev, image, allocator);
		}

		for (VkDeviceMemory memory : node.deletedMemories) {
			vkFreeMemory(dev, memory, allocator);
		}

		for (VkEvent event : node.deletedEvents) {
			vkDestroyEvent(dev, event, allocator);
		}

		if (!(node.hostQueue->flag & IRender::QUEUE_MULTITHREAD)) {
			TSpinLockGuard<uint32_t> guard(node.hostQueue->critical);
			vkFreeCommandBuffers(device->device, node.hostQueue->commandPool, 1, &node.buffer);
		} else {
			vkFreeCommandBuffers(device->device, node.hostQueue->commandPool, 1, &node.buffer);
		}
	}

	void EndCurrentCommandBuffer() {
		if (renderTargetResource != nullptr) {
			vkCmdEndRenderPass(currentCommandBuffer.buffer);
			currentCommandBuffer.commandCount++;
			renderTargetResource = nullptr;
		}

		// printf("End %p at %d\n", currentCommandBuffer.buffer, ::GetCurrentThreadId());
		vkEndCommandBuffer(currentCommandBuffer.buffer);
	}

	void BeginCurrentCommandBuffer() {
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = !(flag & IRender::QUEUE_REPEATABLE) ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
		// printf("Begin %p at %d\n", currentCommandBuffer.buffer, ::GetCurrentThreadId());
		vkBeginCommandBuffer(currentCommandBuffer.buffer, &info);
		currentCommandBuffer.flag = 0;
		currentCommandBuffer.commandCount = 0;
	}

	void FlushCommandBuffer() {
		OPTICK_EVENT();

		EndCurrentCommandBuffer();
		preparedCommandBuffers.Push(std::move(currentCommandBuffer));
		currentCommandBuffer = Allocate();
		FinalizeCommandBuffers();
		BeginCurrentCommandBuffer();
	}

	void FrameBarrier() {
		recycledCommandBuffers.Push(BufferNode());
	}

	void FlushFrame() {
		ClearFrameData<false>();
		FrameBarrier();
	}

	DeviceImplVulkan* device;
	VkCommandPool commandPool;
	uint32_t flag;
	std::atomic<uint32_t> critical;

	// current pipeline states
	IRender::Resource::RenderStateDescription renderStateDescription;
	ResourceBaseImplVulkan* renderTargetResource;

	// command buffer states
	BufferNode currentCommandBuffer;
	TQueueList<BufferNode> preparedCommandBuffers;
	TQueueList<BufferNode> recycledCommandBuffers;
	TQueueList<BufferNode> finalizingCommandBuffers;
	TQueueList<ResourceBaseImplVulkan*> newEvents;
	TQueueList<ResourceBaseImplVulkan*> recycledEvents;
};

void DeviceImplVulkan::CleanupFrameQueue(FrameData& lastFrame) {
	for (size_t k = 0; k < lastFrame.activeQueues.size(); k++) {
		QueueImplVulkan* queue = lastFrame.activeQueues[k];
		// queue->ClearFrameData<false>();
		queue->FlushFrame();
	}

	lastFrame.activeQueues.clear();
}

bool DeviceImplVulkan::DeleteFrameQueue() {
	while (!deletedQueues.Empty()) {
		QueueImplVulkan* queue = deletedQueues.Top();
		deletedQueues.Pop();

		if (queue != nullptr) {
			delete queue;
		} else {
			return true;
		}
	}

	return false;
}

std::vector<String> ZRenderVulkan::EnumerateDevices() {
	std::vector<String> devices;

	uint32_t gpuCount;
	VkResult err = vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
	if (err != VK_SUCCESS || gpuCount == 0) {
		return devices;
	}

	std::vector<VkPhysicalDevice> gpus(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, &gpus[0]);
	devices.resize(gpuCount);

	for (size_t i = 0; i < gpus.size(); i++) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(gpus[i], &prop);
		devices.emplace_back(String(prop.deviceName) + " [" + std::to_string(i) + "]");
	}

	return devices;
}

IRender::Device* ZRenderVulkan::CreateDevice(const String& description) {
	uint32_t gpuCount;
	VkResult err = vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr);
	if (err != VK_SUCCESS || gpuCount == 0) {
		return nullptr;
	}

	std::vector<VkPhysicalDevice> gpus(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, &gpus[0]);

	for (size_t i = 0; i < gpus.size(); i++) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(gpus[i], &prop);
		String name = String(prop.deviceName) + " [" + std::to_string(i) + "]";
		if (name == description || description.empty()) {
			VkPhysicalDevice device = gpus[i];

			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
			std::vector<VkQueueFamilyProperties> queues(count);
			if (count == 0) return nullptr;

			uint32_t family = ~(uint32_t)0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, &queues[0]);
			for (uint32_t i = 0; i < count; i++) {
				if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					family = i;
					break;
				}
			}

			if (family == ~(uint32_t)0) {
				return nullptr;
			}

			const char* deviceExtensions[] = { "VK_KHR_swapchain" };
			int deviceExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
			const float queuePriority[] = { 1.0f };

			VkDeviceQueueCreateInfo queueInfo[1] = {};
			queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo[0].queueFamilyIndex = family;
			queueInfo[0].queueCount = 1;
			queueInfo[0].pQueuePriorities = queuePriority;

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
			createInfo.pQueueCreateInfos = queueInfo;
			createInfo.enabledExtensionCount = deviceExtensionCount;
			createInfo.ppEnabledExtensionNames = deviceExtensions;

			VkAllocationCallbacks* allocator = nullptr; // TODO: customize allocator

			VkDevice logicDevice;
			Verify("create device", vkCreateDevice(device, &createInfo, allocator, &logicDevice));
			VkQueue queue;
			vkGetDeviceQueue(logicDevice, family, 0, &queue);

			VkBool32 res;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, family, (VkSurfaceKHR)surface, &res);

			DeviceImplVulkan* impl = new DeviceImplVulkan(*this, allocator, device, logicDevice, family, queue);

			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			SetDeviceResolution(impl, Int2(w, h));
			BuildSwapChain(impl);

			return impl;
		}
	}

	return nullptr;
}

void ZRenderVulkan::DeleteDevice(IRender::Device* device) {
	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(device);
	delete impl;
}

void ZRenderVulkan::BuildSwapChain(IRender::Device* dev) {
	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(dev);
	VkAllocationCallbacks* allocator = impl->allocator;
	VkSwapchainKHR oldSwapChain = impl->swapChain;
	impl->swapChain = VK_NULL_HANDLE;

	// reset device swap chain
	VkDevice device = impl->device;
	Verify("wait idle", vkDeviceWaitIdle(device));

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = (VkSurfaceKHR)surface;
	info.minImageCount = MIN_IMAGE_COUNT;
	info.imageFormat = impl->surfaceFormat;
	info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // triple buffer
	info.clipped = VK_TRUE;
	info.oldSwapchain = oldSwapChain;

	VkSurfaceCapabilitiesKHR cap;
	Verify("get physical device cap", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl->physicalDevice, (VkSurfaceKHR)surface, &cap));
	if (info.minImageCount < cap.minImageCount)
		info.minImageCount = cap.minImageCount;
	else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
		info.minImageCount = cap.maxImageCount;

	if (cap.currentExtent.width == 0xffffffff) {
		info.imageExtent.width = impl->resolution.x();
		info.imageExtent.height = impl->resolution.y();
	} else {
		info.imageExtent.width = impl->resolution.x() = cap.currentExtent.width;
		info.imageExtent.height = impl->resolution.y() = cap.currentExtent.height;
	}

	// Cleanup old frame data
	impl->DestroyAllFrames(allocator, false);

	Verify("create swap chain", vkCreateSwapchainKHR(device, &info, allocator, &impl->swapChain));
	uint32_t imageCount;
	Verify("get swapchain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, nullptr));

	std::vector<VkImage> images(imageCount);
	Verify("get swap chain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, &images[0]));

	impl->frames.resize(imageCount);
	impl->semaphores.resize(imageCount);

	VkImageViewCreateInfo viewinfo = {};
	viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewinfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	viewinfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewinfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewinfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewinfo.components.a = VK_COMPONENT_SWIZZLE_A;
	VkImageSubresourceRange imageRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewinfo.subresourceRange = imageRange;

	VkFenceCreateInfo fenceinfo = {};
	fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo seminfo = {};
	seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < imageCount; i++) {
		FrameData& frameData = impl->frames[i];
		viewinfo.image = frameData.backBufferImage = images[i];
		Verify("create image view", vkCreateImageView(device, &viewinfo, allocator, &frameData.backBufferView));
		Verify("create fence", vkCreateFence(device, &fenceinfo, allocator, &frameData.fence));

		SemaphoreData& semaphoreData = impl->semaphores[i];
		Verify("create semaphore (acquire)", vkCreateSemaphore(device, &seminfo, allocator, &semaphoreData.acquireSemaphore));
		Verify("create semaphore (release)", vkCreateSemaphore(device, &seminfo, allocator, &semaphoreData.releaseSemaphore));

		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;// flag& IRender::QUEUE_SECONDARY ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = impl->mainCommandPool;
		info.pNext = nullptr;
		vkAllocateCommandBuffers(device, &info, &frameData.mainCommandBuffer);

		// setup barrier for PRESENT layout
		VkImageMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memoryBarrier.image = viewinfo.image;
		memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memoryBarrier.subresourceRange.baseMipLevel = 0;
		memoryBarrier.subresourceRange.levelCount = 1;
		memoryBarrier.subresourceRange.baseArrayLayer = 0;
		memoryBarrier.subresourceRange.layerCount = 1;

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		vkBeginCommandBuffer(frameData.mainCommandBuffer, &beginInfo);
		vkCmdPipelineBarrier(frameData.mainCommandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
		memoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		// vkCmdPipelineBarrier(frameData.mainCommandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
		vkEndCommandBuffer(frameData.mainCommandBuffer);

		TSpinLockGuard<size_t> guard(impl->queueCritical);
		impl->deletedQueues.Push(nullptr);
	}

	if (oldSwapChain)
		vkDestroySwapchainKHR(device, oldSwapChain, allocator);

	impl->swapChainRebuild = false;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* dev, const Int2& resolution) {
	OPTICK_EVENT();

	if (resolution.x() == 0 || resolution.y() == 0)
		return;

	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(dev);
	impl->resolution = resolution;
	impl->swapChainRebuild = true;
}

Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) {
	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(device);
	return impl->resolution;
}

bool ZRenderVulkan::PreDeviceFrame(IRender::Device* device) {
	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(device);
	return impl->PreFrame();
}

void ZRenderVulkan::PostDeviceFrame(IRender::Device* device) {
	DeviceImplVulkan* impl = static_cast<DeviceImplVulkan*>(device);
	impl->PostFrame();
}

IRender::Queue* ZRenderVulkan::CreateQueue(Device* dev, uint32_t flag) {
	return new QueueImplVulkan(static_cast<DeviceImplVulkan*>(dev), flag);
}

IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* q) {
	QueueImplVulkan* queue = static_cast<QueueImplVulkan*>(q);
	return queue->device;
}

size_t ZRenderVulkan::GetProfile(Device* device, const String& feature) {
	return true; // by now all supported.
}

void ZRenderVulkan::SubmitQueues(Queue** queues, uint32_t count, SubmitOption option) {
	if (count != 0) {
		DeviceImplVulkan* device = static_cast<QueueImplVulkan*>(queues[0])->device;
		FrameData& frame = device->GetCurrentFrame();
		SemaphoreData& semaphore = device->GetCurrentSemaphore();
		std::vector<VkCommandBuffer>& commandBuffers = device->frameCommandBuffers;

		for (uint32_t i = 0; i < count; i++) {
			QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queues[i]);
			if (q->preparedCommandBuffers.Empty()) {
				if (option == IRender::SUBMIT_EXECUTE_REPEAT) {
					device->repeatQueueStarvationCount++;
				}
			} else {
				BinaryInsert(frame.activeQueues, q);
			
				switch (option) {
					case IRender::SUBMIT_EXECUTE_ALL:
					{
						do {
							QueueImplVulkan::BufferNode bufferNode = std::move(q->preparedCommandBuffers.Top());
							q->preparedCommandBuffers.Pop();
							if (bufferNode.commandCount != 0) {
								device->queueSummaryFlag |= bufferNode.flag;
								commandBuffers.emplace_back(bufferNode.buffer);
							}

							q->recycledCommandBuffers.Push(std::move(bufferNode));
						} while (!q->preparedCommandBuffers.Empty());
						break;
					}
					case IRender::SUBMIT_EXECUTE_CONSUME:
					{
						QueueImplVulkan::BufferNode bufferNode = std::move(q->preparedCommandBuffers.Top());
						q->preparedCommandBuffers.Pop();
						if (bufferNode.commandCount != 0) {
							device->queueSummaryFlag |= bufferNode.flag;
							commandBuffers.emplace_back(bufferNode.buffer);
						}

						q->recycledCommandBuffers.Push(std::move(bufferNode));
						break;
					}
					case IRender::SUBMIT_EXECUTE_REPEAT:
					{
						// Find latest command buffer
						while (q->preparedCommandBuffers.Count() > (device->swapChainRebuilding ? 0u : 1u)) {
							QueueImplVulkan::BufferNode bufferNode = std::move(q->preparedCommandBuffers.Top());
							q->preparedCommandBuffers.Pop();
							q->recycledCommandBuffers.Push(std::move(bufferNode));
						}

						if (!device->swapChainRebuilding) {
							QueueImplVulkan::BufferNode& bufferNode = q->preparedCommandBuffers.Top();
							if (bufferNode.commandCount != 0) {
								device->queueSummaryFlag |= bufferNode.flag;
								commandBuffers.emplace_back(bufferNode.buffer);
							}
						} else {
							device->repeatQueueStarvationCount++;
						}
						break;
					}
					default:
						assert(false); // not supported
				}
			}
		}
	}
}

void ZRenderVulkan::DeleteQueue(Queue* q) {
	QueueImplVulkan* queue = static_cast<QueueImplVulkan*>(q);
	TSpinLockGuard<size_t> guard(queue->device->queueCritical);
	queue->device->deletedQueues.Push(queue); // delayed delete
}

void ZRenderVulkan::FlushQueue(Queue* q) {
	// start new segment
	QueueImplVulkan* queue = static_cast<QueueImplVulkan*>(q);
	QueueImplVulkan::LockGuard guard(*queue);
	queue->FlushCommandBuffer();
}

bool ZRenderVulkan::IsQueueEmpty(Queue* q) {
	QueueImplVulkan* queue = static_cast<QueueImplVulkan*>(q);
	return queue->preparedCommandBuffers.Empty();
}

static std::pair<uint32_t, VkIndexType> GetFormatSize(uint32_t format) {
	VkIndexType indexType = VK_INDEX_TYPE_UINT32;
	uint32_t elementSize = sizeof(float);

	switch (format) {
		case IRender::Resource::BufferDescription::UNSIGNED_BYTE:
			indexType = VK_INDEX_TYPE_UINT8_EXT;
			elementSize = sizeof(uint8_t);
			break;
		case IRender::Resource::BufferDescription::UNSIGNED_SHORT:
			indexType = VK_INDEX_TYPE_UINT16;
			elementSize = sizeof(uint16_t);
			break;
		case IRender::Resource::BufferDescription::HALF:
			indexType = VK_INDEX_TYPE_UINT16;
			elementSize = sizeof(uint16_t);
			break;
		case IRender::Resource::BufferDescription::FLOAT:
			indexType = VK_INDEX_TYPE_UINT32;
			elementSize = sizeof(float);
			break;
		case IRender::Resource::BufferDescription::UNSIGNED_INT:
			indexType = VK_INDEX_TYPE_UINT32;
			elementSize = sizeof(uint32_t);
			break;
		default:
			assert(false);
			break;
	}

	return std::make_pair(elementSize, indexType);
}

static VkFormat TranslateFormat(uint32_t format, uint32_t layout, uint32_t compressType, uint32_t compressBlock, bool srgb) {
	VkFormat vkFormat = VK_FORMAT_UNDEFINED;

	switch (compressType) {
		case IRender::Resource::TextureDescription::NONE:
		{
			static const VkFormat formats[IRender::Resource::TextureDescription::Layout::END][IRender::Resource::Description::Format::END] = {
				{ VK_FORMAT_R8_UNORM, VK_FORMAT_R16_UNORM, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT },
				{ VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_UINT },
				{ VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_UINT },
				{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_UINT },
				{ VK_FORMAT_UNDEFINED, VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_S8_UINT, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
			};

			static const VkFormat srgbformats[IRender::Resource::TextureDescription::Layout::END][IRender::Resource::Description::Format::END] = {
				{ VK_FORMAT_R8_SRGB, VK_FORMAT_UNDEFINED, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT },
				{ VK_FORMAT_R8G8_SRGB, VK_FORMAT_UNDEFINED, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_UINT },
				{ VK_FORMAT_R8G8B8_SRGB, VK_FORMAT_UNDEFINED, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_UINT },
				{ VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_UNDEFINED, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_UINT },
				{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_S8_UINT, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
				{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
			};

			vkFormat = srgb ? srgbformats[layout][format] : formats[layout][format];
			break;
		}
		case IRender::Resource::TextureDescription::ASTC:
		{
			static const VkFormat unormFormats[IRender::Resource::TextureDescription::BLOCK_END] = {
				VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
				VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
				VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
				VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
				VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
				VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
				VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
			};

			static const VkFormat srgbFormats[IRender::Resource::TextureDescription::BLOCK_END] = {
				VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
				VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
				VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
				VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
				VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
				VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
				VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
			};

			vkFormat = srgb ? srgbFormats[compressBlock] : unormFormats[compressBlock];
			break;
		}
		case IRender::Resource::TextureDescription::BPTC:
		{
			static const VkFormat unormFormats[IRender::Resource::TextureDescription::BLOCK_END] = {
				VK_FORMAT_BC7_UNORM_BLOCK,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED
			};

			static const VkFormat srgbFormats[IRender::Resource::TextureDescription::BLOCK_END] = {
				VK_FORMAT_BC7_SRGB_BLOCK,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED,
				VK_FORMAT_UNDEFINED
			};

			vkFormat = srgb ? srgbFormats[compressBlock] : unormFormats[compressBlock];
			break;
		}
	}

	assert(vkFormat != VK_FORMAT_UNDEFINED);
	return vkFormat;
}

template <class T>
class ResourceImplVulkan {};

template <>
class ResourceImplVulkan<IRender::Resource::TextureDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::TextureDescription, IRender::Resource::RESOURCE_TEXTURE, ResourceImplVulkan<IRender::Resource::TextureDescription> > {
public:
	ResourceImplVulkan() : image(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE), imageViewDepthAspect(VK_NULL_HANDLE), imageSampler(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), dimension(0, 0, 0) {}
	~ResourceImplVulkan() {}

	const void* GetHandle() const {
		return reinterpret_cast<const void*>((size_t)image);
	}

	void Clear(QueueImplVulkan& queue) {
		if (imageSampler != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedSamplers.emplace_back(imageSampler);
			imageSampler = VK_NULL_HANDLE;
		}

		if (imageViewDepthAspect != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedImageViews.emplace_back(imageViewDepthAspect);
			imageViewDepthAspect = VK_NULL_HANDLE;
		}

		if (imageView != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedImageViews.emplace_back(imageView);
			imageView = VK_NULL_HANDLE;
		}

		if (image != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedImages.emplace_back(image);
			image = VK_NULL_HANDLE;
		}

		if (memory != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedMemories.emplace_back(memory);
		}
	}

	void Delete(QueueImplVulkan& queue) {
		Clear(queue);
		delete this;
	}

	static uint32_t GetMipCount(IRender::Resource::TextureDescription& desc) {
		return desc.state.mip != IRender::Resource::TextureDescription::SPECMIP ? 1 : Math::Log2((uint32_t)Math::Min(desc.dimension.x(), desc.dimension.y())) + 1;
	}

	static uint32_t GetLayerCount(IRender::Resource::TextureDescription& desc) {
		return (desc.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? desc.dimension.z() : 1u) * (desc.state.type == IRender::Resource::TextureDescription::TEXTURE_2D_CUBE ? 6u : 1u);
	}

	static UShort2 GetBlockSize(IRender::Resource::TextureDescription& desc) {
		if (!desc.state.compress) {
			return UShort2(1, 1);
		} else {
			switch (desc.state.block) {
				case IRender::Resource::TextureDescription::BLOCK_4X4:
					return UShort2(4, 4);
				case IRender::Resource::TextureDescription::BLOCK_5X4:
					return UShort2(5, 4);
				case IRender::Resource::TextureDescription::BLOCK_5X5:
					return UShort2(5, 5);
				case IRender::Resource::TextureDescription::BLOCK_6X5:
					return UShort2(6, 5);
				case IRender::Resource::TextureDescription::BLOCK_6X6:
					return UShort2(6, 6);
				case IRender::Resource::TextureDescription::BLOCK_8X5:
					return UShort2(8, 5);
				case IRender::Resource::TextureDescription::BLOCK_8X6:
					return UShort2(8, 6);
				case IRender::Resource::TextureDescription::BLOCK_10X5:
					return UShort2(10, 5);
				case IRender::Resource::TextureDescription::BLOCK_10X6:
					return UShort2(10, 6);
				case IRender::Resource::TextureDescription::BLOCK_8X8:
					return UShort2(8, 8);
				case IRender::Resource::TextureDescription::BLOCK_10X8:
					return UShort2(10, 8);
				case IRender::Resource::TextureDescription::BLOCK_10X10:
					return UShort2(10, 10);
				case IRender::Resource::TextureDescription::BLOCK_12X10:
					return UShort2(12, 10);
				case IRender::Resource::TextureDescription::BLOCK_12X12:
					return UShort2(12, 12);
			}

			assert(false);
			return UShort2(4, 4);
		}
	}

	template <class T>
	static void Shift(T& t) {
		if (t > 1) t >>= 1;
	}

	static VkFilter ConvertImageFilter(uint32_t filter) {
		switch (filter) {
			case IRender::Resource::TextureDescription::POINT:
				return VK_FILTER_NEAREST;
			case IRender::Resource::TextureDescription::LINEAR:
				return VK_FILTER_LINEAR;
				/*
			case IRender::Resource::TextureDescription::ANSOTRIPIC:
			case IRender::Resource::TextureDescription::TRILINEAR:
				return VK_FILTER_LINEAR;*/
		}

		return VK_FILTER_LINEAR;
	}

	static VkSamplerAddressMode ConvertAddressMode(uint32_t address) {
		switch (address) {
			case IRender::Resource::TextureDescription::REPEAT:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case IRender::Resource::TextureDescription::CLAMP:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case IRender::Resource::TextureDescription::MIRROR_REPEAT:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case IRender::Resource::TextureDescription::MIRROR_CLAMP:
				return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	void Upload(QueueImplVulkan& queue) {
		DeviceImplVulkan* device = queue.device;
		IRender::Resource::TextureDescription& desc = description;

		if (desc.dimension != dimension) {
			Clear(queue);

			dimension = desc.dimension;
			assert(dimension.x() != 0);
			assert(dimension.y() != 0);
			assert(dimension.z() != 0);

			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = ConvertImageFilter(desc.state.sample);
			samplerInfo.minFilter =  ConvertImageFilter(desc.state.sample);
			samplerInfo.mipmapMode = desc.state.sample == IRender::Resource::TextureDescription::TRILINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
			samplerInfo.addressModeU = ConvertAddressMode(desc.state.addressU);
			samplerInfo.addressModeV = ConvertAddressMode(desc.state.addressV);
			samplerInfo.addressModeW = ConvertAddressMode(desc.state.addressW);
			samplerInfo.minLod = -1000;
			samplerInfo.maxLod = 1000;
			samplerInfo.maxAnisotropy = 1.0f;
			Verify("create sampler", vkCreateSampler(device->device, &samplerInfo, device->allocator, &imageSampler));

			VkImageCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			switch (desc.state.type) {
			case IRender::Resource::TextureDescription::TEXTURE_1D:
				info.imageType = VK_IMAGE_TYPE_1D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_2D:
				info.imageType = VK_IMAGE_TYPE_2D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_2D_CUBE:
				info.imageType = VK_IMAGE_TYPE_2D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_3D:
				info.imageType = VK_IMAGE_TYPE_3D;
				break;
			}

			bool isDepthStencil = desc.state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL || desc.state.layout == IRender::Resource::TextureDescription::DEPTH || desc.state.layout == IRender::Resource::TextureDescription::STENCIL;

			info.format = TranslateFormat(desc.state.format, desc.state.layout, desc.state.compress, desc.state.block, false);
			info.extent.width = desc.dimension.x();
			info.extent.height = desc.dimension.y();
			info.extent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? desc.dimension.z() : 1u;
			info.mipLevels = GetMipCount(desc);
			info.arrayLayers = GetLayerCount(desc);
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = desc.state.attachment ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | (isDepthStencil ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) : VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			info.flags = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_2D_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

			Verify("create image", vkCreateImage(device->device, &info, device->allocator, &image));

			VkMemoryRequirements req;
			vkGetImageMemoryRequirements(device->device, image, &req);

			VkMemoryAllocateInfo allocInfo;
			allocInfo.pNext = nullptr;
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = device->GetMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);

			// printf("Memory size: %x", req.size);
			Verify("allocate texture memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &memory));
			Verify("bind image", vkBindImageMemory(device->device, image, memory, 0));

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = info.format;
			viewInfo.subresourceRange.aspectMask = desc.state.layout == IRender::Resource::TextureDescription::DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : desc.state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : desc.state.layout == IRender::Resource::TextureDescription::STENCIL ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = info.mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = info.arrayLayers;
			Verify("create image view", vkCreateImageView(device->device, &viewInfo, device->allocator, &imageView));

			if (desc.state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL) {
				viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				Verify("create image view", vkCreateImageView(device->device, &viewInfo, device->allocator, &imageViewDepthAspect));
			}
		}

		if (!desc.data.Empty()) {
			assert(!desc.state.attachment);
			size_t size = desc.data.GetSize();
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer uploadBuffer;
			Verify("create buffer", vkCreateBuffer(device->device, &bufferInfo, device->allocator, &uploadBuffer));

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device->device, uploadBuffer, &req);
			assert(req.size >= size);
			// assert(((size_t)description.data.GetData() & ~req.alignment) == 0);
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = device->GetMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

			VkDeviceMemory bufferMemory;
			Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &bufferMemory));
			Verify("bind memory", vkBindBufferMemory(device->device, uploadBuffer, bufferMemory, 0));

			void* map = nullptr;
			Verify("map memory", vkMapMemory(device->device, bufferMemory, 0, size, 0, &map));
			memcpy(map, desc.data.GetData(), size);
			VkMappedMemoryRange range = {};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = bufferMemory;
			range.size = size;
			Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
			vkUnmapMemory(device->device, bufferMemory);

			// Copy buffer to image
			VkImageMemoryBarrier copyBarrier = {};
			copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copyBarrier.image = image;
			copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyBarrier.subresourceRange.baseMipLevel = 0;
			copyBarrier.subresourceRange.levelCount = GetMipCount(desc);
			copyBarrier.subresourceRange.baseArrayLayer = 0;
			copyBarrier.subresourceRange.layerCount = GetLayerCount(desc);
			vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);
			queue.currentCommandBuffer.commandCount++;

			uint32_t mipCount = GetMipCount(desc);
			uint32_t offset = 0;

			// TODO: only support 1/1 compression now
			uint32_t bitDepth = desc.state.compress ? 8 : IImage::GetPixelBitDepth((IRender::Resource::TextureDescription::Format)desc.state.format, (IRender::Resource::TextureDescription::Layout)desc.state.layout);
			uint32_t height = desc.dimension.x();
			uint32_t width = desc.dimension.y();
			uint32_t depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? desc.dimension.z() : 1u;
			uint32_t layer = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? 1u : desc.dimension.z();
			UShort2 blockSize = GetBlockSize(desc);

			std::vector<VkBufferImageCopy> regions;
			regions.reserve(mipCount * layer);
			for (uint32_t k = 0; k < layer; k++) {
				for (uint32_t i = 0; i < mipCount; i++) {
					VkBufferImageCopy region = {};
					region.bufferOffset = offset;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;
					region.imageSubresource.mipLevel = i;
					region.imageExtent.width = (width + blockSize.x() - 1) / blockSize.x() * blockSize.y();
					region.imageExtent.height = (height + blockSize.y() - 1) / blockSize.y() * blockSize.y();
					region.imageExtent.depth = depth;
					regions.emplace_back(region);

					uint32_t diff = width * height * depth * bitDepth / 8;
					if (offset + diff < size)
						offset += diff;

					Shift(width);
					Shift(height);
					Shift(depth);
				}
			}

			vkCmdCopyBufferToImage(queue.currentCommandBuffer.buffer, uploadBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, verify_cast<uint32_t>(regions.size()), &regions[0]);
			queue.currentCommandBuffer.commandCount++;

			VkImageMemoryBarrier useBarrier = {};
			useBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.image = image;
			useBarrier.subresourceRange = copyBarrier.subresourceRange;

			vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &useBarrier);
			queue.currentCommandBuffer.commandCount++;

			queue.currentCommandBuffer.deletedBuffers.emplace_back(uploadBuffer);
			queue.currentCommandBuffer.deletedMemories.emplace_back(bufferMemory);
			desc.data.Clear();

			// vkDestroyBuffer(device->device, uploadBuffer, device->allocator);
			// vkFreeMemory(device->device, bufferMemory, device->allocator);
		}
	}

	VkImage image;
	VkImageView imageView;
	VkImageView imageViewDepthAspect; // for shader read depth
	VkSampler imageSampler;
	VkDeviceMemory memory;
	UShort3 dimension;
};

template <>
class ResourceImplVulkan<IRender::Resource::BufferDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::BufferDescription, IRender::Resource::RESOURCE_BUFFER, ResourceImplVulkan<IRender::Resource::BufferDescription> > {
public:
	ResourceImplVulkan() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), memorySize(0) {}
	~ResourceImplVulkan() {
		assert(buffer == VK_NULL_HANDLE);
		assert(memory == VK_NULL_HANDLE);
	}

	const void* GetHandle() const {
		return reinterpret_cast<const void*>((size_t)buffer);
	}

	void Clear(QueueImplVulkan& q) {
		if (buffer != VK_NULL_HANDLE) {
			q.currentCommandBuffer.deletedBuffers.emplace_back(buffer);
			buffer = VK_NULL_HANDLE;
		}

		if (memory != VK_NULL_HANDLE) {
			q.currentCommandBuffer.deletedMemories.emplace_back(memory);
			memory = VK_NULL_HANDLE;
		}
	}

	void Delete(QueueImplVulkan& queue) {
		Clear(queue);

		delete this;
	}

	void Upload(QueueImplVulkan& queue) {
		DeviceImplVulkan* device = queue.device;
		IRender::Resource::BufferDescription& desc = description;

		size_t size = desc.state.length == 0 ? desc.data.GetSize() : desc.state.length;
		if (memorySize != size) {
			Clear(queue);
			memorySize = size;

			VkBufferCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			createInfo.size = size;
			switch (desc.state.usage) {
				case IRender::Resource::BufferDescription::INDEX:
					createInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
					break;
				case IRender::Resource::BufferDescription::VERTEX:
					createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
					break;
				case IRender::Resource::BufferDescription::INSTANCED:
					assert(desc.state.dynamic); // INSTANCED buffer should be dynamic
					createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
					break;
				case IRender::Resource::BufferDescription::UNIFORM:
					createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
					break;
				case IRender::Resource::BufferDescription::STORAGE:
					createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
					break;
			}

			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			Verify("create buffer", vkCreateBuffer(device->device, &createInfo, device->allocator, &buffer));
			
			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device->device, buffer, &req);
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = device->GetMemoryType(desc.state.dynamic ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);

			Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &memory));
			Verify("bind memory", vkBindBufferMemory(device->device, buffer, memory, 0));
		}

		void* map = nullptr;
		if (size != 0) {
			if (!desc.state.dynamic) {
				size_t size = desc.data.GetSize();

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				VkBuffer uploadBuffer;
				Verify("create buffer", vkCreateBuffer(device->device, &bufferInfo, device->allocator, &uploadBuffer));

				VkMemoryRequirements req;
				vkGetBufferMemoryRequirements(device->device, uploadBuffer, &req);
				assert(req.size >= size);
				// assert(((size_t)description.data.GetData() & ~req.alignment) == 0);
				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = req.size;
				allocInfo.memoryTypeIndex = device->GetMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);

				VkDeviceMemory bufferMemory;
				Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &bufferMemory));
				Verify("bind memory", vkBindBufferMemory(device->device, uploadBuffer, bufferMemory, 0));

				if (!desc.data.Empty()) {
					void* map = nullptr;
					Verify("map memory", vkMapMemory(device->device, bufferMemory, 0, size, 0, &map));
					memcpy(map, desc.data.GetData(), size);
					VkMappedMemoryRange range = {};
					range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					range.memory = bufferMemory;
					range.size = size;
					Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
					vkUnmapMemory(device->device, bufferMemory);
				}

				VkBufferMemoryBarrier copyBarrier = {};
				copyBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copyBarrier.buffer = uploadBuffer;
				copyBarrier.offset = 0;
				copyBarrier.size = size;

				vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &copyBarrier, 0, nullptr);
				queue.currentCommandBuffer.commandCount++;

				VkBufferCopy bufferCopy = {};
				bufferCopy.srcOffset = 0;
				bufferCopy.dstOffset = 0;
				bufferCopy.size = size;

				vkCmdCopyBuffer(queue.currentCommandBuffer.buffer, uploadBuffer, buffer, 1, &bufferCopy);
				queue.currentCommandBuffer.commandCount++;

				VkBufferMemoryBarrier useBarrier = {};
				useBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
				useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				useBarrier.buffer = buffer;
				useBarrier.offset = 0;
				useBarrier.size = size;

				vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &useBarrier, 0, nullptr);
				queue.currentCommandBuffer.commandCount++;
				queue.currentCommandBuffer.deletedBuffers.emplace_back(uploadBuffer);
				queue.currentCommandBuffer.deletedMemories.emplace_back(bufferMemory);
			} else if (!desc.data.Empty()) {
				Verify("map memory", vkMapMemory(device->device, memory, 0, size, 0, &map));
				memcpy(map, desc.data.GetData(), size);
				VkMappedMemoryRange range = {};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.memory = memory;
				range.size = size;
				Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
				vkUnmapMemory(device->device, memory);
			}

			desc.data.Clear();
		}
	}

	VkBuffer buffer;
	VkDeviceMemory memory;
	size_t memorySize;
};

class VulkanDrawCallResourceBase {
public:
	VulkanDrawCallResourceBase() : descriptorSetTexture(VK_NULL_HANDLE), descriptorSetUniformBuffer(VK_NULL_HANDLE), descriptorSetStorageBuffer(VK_NULL_HANDLE) {}

	void Delete(QueueImplVulkan& queue) {
		if (descriptorSetTexture != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptors.emplace_back(queue.device->descriptorAllocators[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER], descriptorSetTexture);
			descriptorSetTexture = VK_NULL_HANDLE;
		}

		if (descriptorSetUniformBuffer != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptors.emplace_back(queue.device->descriptorAllocators[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER], descriptorSetUniformBuffer);
			descriptorSetUniformBuffer = VK_NULL_HANDLE;
		}

		if (descriptorSetStorageBuffer != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptors.emplace_back(queue.device->descriptorAllocators[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER], descriptorSetStorageBuffer);
			descriptorSetStorageBuffer = VK_NULL_HANDLE;
		}

		// delete this;
	}

	void UploadImpl(QueueImplVulkan& queue, IRender::Resource* shaderResource, IRender::Resource::DrawCallDescription::BufferRange* bufferResources, uint32_t bufferCount, IRender::Resource** textureResources, uint32_t textureCount);
	void ExecuteImpl(QueueImplVulkan& queue, IRender::Resource* shaderResource, IRender::Resource::DrawCallDescription::BufferRange* indexBuffer, IRender::Resource::DrawCallDescription::BufferRange* bufferResources, uint32_t bufferCount, IRender::Resource** textureResources, uint32_t textureCount, const UInt3& instanceCounts);

	VkDescriptorSet descriptorSetTexture;
	VkDescriptorSet descriptorSetUniformBuffer;
	VkDescriptorSet descriptorSetStorageBuffer;

	Bytes drawSignature;
};

template <>
class ResourceImplVulkan<IRender::Resource::DrawCallDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::DrawCallDescription, IRender::Resource::RESOURCE_DRAWCALL, ResourceImplVulkan<IRender::Resource::DrawCallDescription> >, public VulkanDrawCallResourceBase {
public:
	void Upload(QueueImplVulkan& queue);
	void Execute(QueueImplVulkan& queue);

	void Delete(QueueImplVulkan& queue) {
		VulkanDrawCallResourceBase::Delete(queue);
		delete this;
	}
};

template <>
class ResourceImplVulkan<IRender::Resource::QuickDrawCallDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::QuickDrawCallDescription, IRender::Resource::RESOURCE_QUICK_DRAWCALL, ResourceImplVulkan<IRender::Resource::QuickDrawCallDescription> >, public VulkanDrawCallResourceBase {
public:
	void Upload(QueueImplVulkan& queue);
	void Execute(QueueImplVulkan& queue);

	void Delete(QueueImplVulkan& queue) {
		VulkanDrawCallResourceBase::Delete(queue);
		delete this;
	}
};

template <>
class ResourceImplVulkan<IRender::Resource::EventDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::EventDescription, IRender::Resource::RESOURCE_EVENT, ResourceImplVulkan<IRender::Resource::EventDescription> > {
public:
	ResourceImplVulkan() : eventHandle(VK_NULL_HANDLE) {
		downloadDescription.store(&description, std::memory_order_release);
	}

	void Upload(QueueImplVulkan& queue) {
		if (eventHandle == VK_NULL_HANDLE) {
			VkEventCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;

			Verify("create event", vkCreateEvent(queue.device->device, &info, queue.device->allocator, &eventHandle));
		}
	}

	void SynchronizeDownload(QueueImplVulkan& queue) {
		if (vkGetEventStatus(queue.device->device, eventHandle) == VK_EVENT_SET) {
			description.counter++;
		}
	}

	void Download(QueueImplVulkan& queue) {
		assert(false);
	}

	void Delete(QueueImplVulkan& queue) {
		if (eventHandle != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedEvents.emplace_back(eventHandle);
		}

		delete this;
	}

	void Execute(QueueImplVulkan& queue) {
		assert(eventHandle != VK_NULL_HANDLE);
		vkCmdSetEvent(queue.currentCommandBuffer.buffer, eventHandle, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		queue.currentCommandBuffer.commandCount++;
		if (description.eventCallback) {
			queue.newEvents.Push(this);
		}
	}

	VkEvent eventHandle;
};

struct PipelineInstance {
	VkPipeline pipeline;
};

static VkCompareOp ConvertCompareOperation(uint32_t op) {
	switch (op) {
	case IRender::Resource::RenderStateDescription::DISABLED:
		return VK_COMPARE_OP_NEVER;
	case IRender::Resource::RenderStateDescription::ALWAYS:
		return VK_COMPARE_OP_ALWAYS;
	case IRender::Resource::RenderStateDescription::LESS:
		return VK_COMPARE_OP_LESS;
	case IRender::Resource::RenderStateDescription::LESS_EQUAL:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case IRender::Resource::RenderStateDescription::GREATER:
		return VK_COMPARE_OP_GREATER;
	case IRender::Resource::RenderStateDescription::GREATER_EQUAL:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case IRender::Resource::RenderStateDescription::EQUAL:
		return VK_COMPARE_OP_EQUAL;
	}

	return VK_COMPARE_OP_ALWAYS;
}

class PipelineKey {
public:
	bool operator < (const PipelineKey& rhs) const {
		int n = memcmp(&renderState, &rhs.renderState, sizeof(renderState));
		if (n != 0) {
			return n < 0;
		} else if (renderTargetSignature < rhs.renderTargetSignature) {
			return true;
		} else if (renderTargetSignature > rhs.renderTargetSignature) {
			return false;
		} else {
			return bufferSignature < rhs.bufferSignature;
		}
	}

	IRender::Resource::RenderStateDescription renderState;
	uint32_t renderTargetSignature;
	Bytes bufferSignature;
};

template <>
class ResourceImplVulkan<IRender::Resource::RenderStateDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::RenderStateDescription, IRender::Resource::RESOURCE_RENDERSTATE, ResourceImplVulkan<IRender::Resource::RenderStateDescription> > {
public:
	void Execute(QueueImplVulkan& queue) {
		queue.renderStateDescription = description;
	}

	void Delete(QueueImplVulkan& queue) {
		delete this;
	}
};

static uint32_t EncodeRenderTargetSignature(const IRender::Resource::RenderTargetDescription& desc) {
	uint32_t sig = 0;
	sig = (sig << 3) | (desc.depthStorage.loadOp << 1) | (desc.depthStorage.storeOp);
	sig = (sig << 3) | (desc.stencilStorage.loadOp << 1) | (desc.stencilStorage.storeOp);

	for (size_t i = 0; i < desc.colorStorages.size(); i++) {
		const IRender::Resource::RenderTargetDescription::Storage& s = desc.colorStorages[i];
		assert(s.loadOp < 4 && s.storeOp < 2);
		sig = (sig << 3) | (s.loadOp << 1) | (s.storeOp);
	}

	return sig;
}

template <>
class ResourceImplVulkan<IRender::Resource::RenderTargetDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::RenderTargetDescription, IRender::Resource::RESOURCE_RENDERTARGET, ResourceImplVulkan<IRender::Resource::RenderTargetDescription> > {
public:
	ResourceImplVulkan() : signature(0), renderPass(VK_NULL_HANDLE), frameBuffer(VK_NULL_HANDLE) {}

	static VkAttachmentLoadOp ConvertLoadOp(uint32_t k) {
		switch (k) {
		case IRender::Resource::RenderTargetDescription::DEFAULT:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case IRender::Resource::RenderTargetDescription::DISCARD:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case IRender::Resource::RenderTargetDescription::CLEAR:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		assert(false);
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}

	static VkAttachmentStoreOp ConvertStoreOp(uint32_t k) {
		switch (k) {
		case IRender::Resource::RenderTargetDescription::DEFAULT:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case IRender::Resource::RenderTargetDescription::DISCARD:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		assert(false);
		return VK_ATTACHMENT_STORE_OP_STORE;
	}

	void Clear(QueueImplVulkan& queue) {
		if (renderPass != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedRenderPasses.emplace_back(renderPass);
			renderPass = VK_NULL_HANDLE;
		}

		if (frameBuffer != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedFramebuffers.emplace_back(frameBuffer);
			frameBuffer = VK_NULL_HANDLE;
		}
	}

	void Delete(QueueImplVulkan& queue) {
		Clear(queue);
		delete this;
	}

	void Upload(QueueImplVulkan& queue) {
		Clear(queue);

		signature = EncodeRenderTargetSignature(description);
		std::vector<VkAttachmentDescription> attachmentDescriptions(description.colorStorages.size());
		std::vector<VkAttachmentReference> attachmentReferences(attachmentDescriptions.size());
		uint32_t frameBufferCount = 1;
		DeviceImplVulkan* device = queue.device;

		size_t i;
		for (i = 0; i < description.colorStorages.size(); i++) {
			const IRender::Resource::RenderTargetDescription::Storage& storage = description.colorStorages[i];
			VkAttachmentDescription& desc = attachmentDescriptions[i];

			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = ConvertLoadOp(storage.loadOp);
			desc.storeOp = ConvertStoreOp(storage.storeOp);
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			ResourceImplVulkan<IRender::Resource::TextureDescription>* resource = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(storage.resource);

			desc.format = TranslateFormat(resource->description.state.format, resource->description.state.layout, 0, 0, false);

			VkAttachmentReference& colorReference = attachmentReferences[i];
			colorReference.attachment = verify_cast<uint32_t>(i);
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		// TODO: isolate stencil?
		uint32_t colorAttachmentCount = verify_cast<uint32_t>(i);
		assert(description.stencilStorage.resource == nullptr);
		if (description.depthStorage.resource != nullptr) {
			IRender::Resource::RenderTargetDescription::Storage& rt = description.depthStorage;
			IRender::Resource::TextureDescription& desc = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(rt.resource)->description;
			VkAttachmentDescription attachment = {};
			attachment.format = TranslateFormat(desc.state.format, desc.state.layout, desc.state.compress, desc.state.compress, desc.state.srgb);
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = ConvertLoadOp(rt.loadOp);
			attachment.storeOp = ConvertStoreOp(rt.storeOp);
			attachment.stencilLoadOp = ConvertLoadOp(rt.loadOp);
			attachment.stencilStoreOp = ConvertStoreOp(rt.storeOp);
			attachment.initialLayout = attachment.finalLayout =
				desc.state.layout == IRender::Resource::TextureDescription::DEPTH_STENCIL ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
				desc.state.layout == IRender::Resource::TextureDescription::DEPTH ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL :
				VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference reference;
			reference.attachment = verify_cast<uint32_t>(i++);
			reference.layout = attachment.finalLayout;

			attachmentDescriptions.emplace_back(std::move(attachment));
			attachmentReferences.emplace_back(std::move(reference));
		}

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPass.colorAttachmentCount = colorAttachmentCount;
		subPass.pColorAttachments = attachmentReferences.empty() ? nullptr : &attachmentReferences[0];
		subPass.pDepthStencilAttachment = description.depthStorage.resource == nullptr ? nullptr : &attachmentReferences[colorAttachmentCount];

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = verify_cast<uint32_t>(attachmentDescriptions.size());
		renderPassInfo.pAttachments = attachmentDescriptions.empty() ? nullptr : &attachmentDescriptions[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		Verify("create render pass", vkCreateRenderPass(device->device, &renderPassInfo, device->allocator, &renderPass));

		std::vector<VkImageView> attachments(attachmentDescriptions.size());
		for (size_t i = 0; i < attachments.size(); i++) {
			const IRender::Resource::RenderTargetDescription::Storage& storage = i >= colorAttachmentCount ? description.depthStorage : description.colorStorages[i];
			ResourceImplVulkan<IRender::Resource::TextureDescription>* texture = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(storage.resource);
			attachments[i] = texture->imageView;
		}

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = verify_cast<uint32_t>(attachments.size());
		info.pAttachments = attachments.empty() ? nullptr : &attachments[0];
		info.layers = 1;

		ResourceImplVulkan<TextureDescription>* texture = static_cast<ResourceImplVulkan<TextureDescription>*>(description.colorStorages.empty() ? description.depthStorage.resource : description.colorStorages[0].resource);
		info.width = texture->description.dimension.x();
		info.height = texture->description.dimension.y();

		Verify("create framebuffer(s)", vkCreateFramebuffer(device->device, &info, device->allocator, &frameBuffer));
	}

	void Execute(QueueImplVulkan& queue) {
		if (queue.renderTargetResource != nullptr) {
			vkCmdEndRenderPass(queue.currentCommandBuffer.buffer);
			queue.currentCommandBuffer.commandCount++;
		}

		queue.renderTargetResource = this;
		DeviceImplVulkan* device = queue.device;

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = renderPass;
		info.framebuffer = frameBuffer;
		info.renderArea.offset.x = description.range.first.x();
		info.renderArea.offset.y = description.range.second.y();

		UShort2Pair range = description.range;

		if (range.second.x() == 0) {
			info.renderArea.extent.width = description.dimension.x() - range.first.x();
		} else {
			info.renderArea.extent.width = range.second.x() - range.first.x();
		}

		if (range.second.y() == 0) {
			info.renderArea.extent.height = description.dimension.y() - range.first.y();
		} else {
			info.renderArea.extent.height = range.second.y() - range.first.y();
		}

		std::vector<VkClearValue> clearValue(description.colorStorages.size() + (description.depthStorage.resource == nullptr ? 0 : 1));
		for (size_t i = 0; i < clearValue.size(); i++) {
			VkClearValue& cv = clearValue[i];
			IRender::Resource::RenderTargetDescription::Storage& s = i < description.colorStorages.size() ? description.colorStorages[i] : description.depthStorage;
			static_assert(sizeof(cv.color.float32) == sizeof(s.clearColor), "Invalid color size");
			memcpy(&cv.color.float32, &s.clearColor, sizeof(s.clearColor));
			// cv.color = VkClearColorValue{ 0, 0, 1, 1 };
		}

		info.clearValueCount = verify_cast<uint32_t>(clearValue.size());
		info.pClearValues = clearValue.empty() ? nullptr : &clearValue[0];

		VkViewport viewport;
		viewport.x = (float)info.renderArea.offset.x;
		viewport.y = (float)info.renderArea.offset.y;
		viewport.width = (float)info.renderArea.extent.width;
		viewport.height = (float)info.renderArea.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdBeginRenderPass(queue.currentCommandBuffer.buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		queue.currentCommandBuffer.commandCount++;
		vkCmdSetViewport(queue.currentCommandBuffer.buffer, 0, 1, &viewport);
		queue.currentCommandBuffer.commandCount++;

		VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = info.renderArea.extent.width;
		scissor.extent.height = info.renderArea.extent.height;
		vkCmdSetScissor(queue.currentCommandBuffer.buffer, 0, 1, &scissor);
		queue.currentCommandBuffer.commandCount++;
	}

	uint32_t signature;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;
};

template <>
class ResourceImplVulkan<IRender::Resource::ShaderDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::ShaderDescription, IRender::Resource::RESOURCE_SHADER, ResourceImplVulkan<IRender::Resource::ShaderDescription> > {
public:
	ResourceImplVulkan() : descriptorSetLayoutTexture(VK_NULL_HANDLE), descriptorSetLayoutUniformBuffer(VK_NULL_HANDLE), descriptorSetLayoutStorageBuffer(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {
		memset(shaderModules, 0, sizeof(shaderModules));
	}

	void Clear(QueueImplVulkan& queue) {
		if (pipelineLayout != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedPipelineLayouts.emplace_back(pipelineLayout);
			pipelineLayout = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutTexture != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptorSetLayouts.emplace_back(descriptorSetLayoutTexture);
			descriptorSetLayoutTexture = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutUniformBuffer != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptorSetLayouts.emplace_back(descriptorSetLayoutUniformBuffer);
			descriptorSetLayoutUniformBuffer = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutStorageBuffer != VK_NULL_HANDLE) {
			queue.currentCommandBuffer.deletedDescriptorSetLayouts.emplace_back(descriptorSetLayoutStorageBuffer);
			descriptorSetLayoutStorageBuffer = VK_NULL_HANDLE;
		}

		for (size_t k = 0; k < stateInstances.size(); k++) {
			queue.currentCommandBuffer.deletedPipelines.emplace_back(stateInstances[k].second.pipeline);
		}

		stateInstances.clear();

		for (size_t i = 0; i < sizeof(shaderModules) / sizeof(shaderModules[0]); i++) {
			VkShaderModule& m = shaderModules[i];
			if (m != VK_NULL_HANDLE) {
				queue.currentCommandBuffer.deletedShaderModules.emplace_back(m);
				m = VK_NULL_HANDLE;
			}
		}
	}

	PipelineInstance& QueryInstance(QueueImplVulkan* queue, Bytes& signature, IRender::Resource::DrawCallDescription::BufferRange* bufferResources, uint32_t bufferCount) {
		// Generate vertex format.
		const IRender::Resource::RenderStateDescription& renderState = queue->renderStateDescription;
		PipelineKey key;
		key.renderState = renderState;
		key.bufferSignature = signature;
		assert(queue->renderTargetResource != nullptr);
		ResourceImplVulkan<IRender::Resource::RenderTargetDescription>* target = static_cast<ResourceImplVulkan<IRender::Resource::RenderTargetDescription>*>(queue->renderTargetResource);
		assert(target != nullptr);
		key.renderTargetSignature = target->signature;

		std::vector<KeyValue<PipelineKey, PipelineInstance> >::iterator it = BinaryFind(stateInstances.begin(), stateInstances.end(), key);
		if (it != stateInstances.end()) {
			return it->second;
		}

		PipelineInstance instance = {};
		VkPipelineDepthStencilStateCreateInfo depthInfo = {};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthInfo.depthTestEnable = renderState.depthTest != IRender::Resource::RenderStateDescription::DISABLED;
		depthInfo.depthWriteEnable = renderState.depthWrite;
		depthInfo.depthCompareOp = ConvertCompareOperation(renderState.depthTest);
		depthInfo.depthBoundsTestEnable = false;
		depthInfo.stencilTestEnable = renderState.stencilTest != IRender::Resource::RenderStateDescription::DISABLED;
		depthInfo.front.compareOp = ConvertCompareOperation(renderState.stencilTest);
		depthInfo.front.compareMask = renderState.stencilMask;
		depthInfo.front.depthFailOp = renderState.stencilReplaceZFail ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.failOp = renderState.stencilReplaceFail ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.passOp = renderState.stencilReplacePass ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.reference = renderState.stencilValue;
		depthInfo.front.writeMask = renderState.stencilWrite ? 0xFFFFFFFF : 0;
		depthInfo.back = depthInfo.front;
		depthInfo.minDepthBounds = 0;
		depthInfo.maxDepthBounds = 1;

		std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions;
		uint32_t location = 0;
		uint32_t binding = 0;

		for (uint32_t k = 0; k < bufferCount; k++) {
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = bufferResources[k];
			ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
			IRender::Resource::BufferDescription& desc = buffer->description;
			if (desc.state.usage == IRender::Resource::BufferDescription::VERTEX || desc.state.usage == IRender::Resource::BufferDescription::INSTANCED) {
				VkVertexInputBindingDescription bindingDesc = {};
				bindingDesc.binding = binding++;
				bindingDesc.inputRate = desc.state.usage == IRender::Resource::BufferDescription::INSTANCED ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				uint32_t component = bufferRange.component == 0 ? desc.state.component : bufferRange.component;
				bindingDesc.stride = desc.state.stride == 0 ? GetFormatSize(desc.state.format).first * component : desc.state.stride;
				inputBindingDescriptions.emplace_back(bindingDesc);

				// process component > 4
				assert(component != 0);
				for (uint32_t n = 0; n < component; n += 4) {
					VkVertexInputAttributeDescription attributeDesc = {};
					attributeDesc.location = location++;
					attributeDesc.binding = k;
					attributeDesc.format = TranslateFormat(desc.state.format, Math::Min(4u, component - n) - 1, 0, 0, false);
					attributeDesc.offset = n * GetFormatSize(desc.state.format).first;
					inputAttributeDescriptions.emplace_back(std::move(attributeDesc));
				}
			}
		}

		assert(!inputBindingDescriptions.empty());
		assert(!inputAttributeDescriptions.empty());

		VkPipelineVertexInputStateCreateInfo vertexInfo = {};
		vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInfo.vertexBindingDescriptionCount = verify_cast<uint32_t>(inputBindingDescriptions.size());
		vertexInfo.pVertexBindingDescriptions = &inputBindingDescriptions[0];
		vertexInfo.vertexAttributeDescriptionCount = verify_cast<uint32_t>(inputAttributeDescriptions.size());
		vertexInfo.pVertexAttributeDescriptions = &inputAttributeDescriptions[0];

		VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
		iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterInfo = {};
		rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterInfo.polygonMode = renderState.fill ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
		rasterInfo.cullMode = renderState.cull ? renderState.cullFrontFace ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
		rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo msInfo = {};
		msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		// TODO: MSAA
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendStateCreateInfo blendInfo = {};
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.attachmentCount = verify_cast<uint32_t>(target->description.colorStorages.size());
		// TODO: different blend operations
		// create state instance
		VkPipelineColorBlendAttachmentState blendState = {};
		blendState.blendEnable = 0;// renderState.blend;
		blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendState.colorBlendOp = VK_BLEND_OP_ADD;
		blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendState.colorWriteMask = renderState.colorWrite ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT : 0;

		std::vector<VkPipelineColorBlendAttachmentState> blendStates(blendInfo.attachmentCount, blendState);
		blendInfo.pAttachments = blendInfo.attachmentCount == 0 ? nullptr : &blendStates[0];

		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamicState.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = 0;
		info.stageCount = verify_cast<uint32_t>(shaderStageCreateInfos.size());
		info.pStages = &shaderStageCreateInfos[0];
		info.pVertexInputState = &vertexInfo;
		info.pInputAssemblyState = &iaInfo;
		info.pViewportState = &viewportInfo;
		info.pRasterizationState = &rasterInfo;
		info.pMultisampleState = &msInfo;
		info.pDepthStencilState = &depthInfo;
		info.pColorBlendState = &blendInfo;
		info.pDynamicState = &dynamicState;
		info.layout = pipelineLayout;
		info.renderPass = target->renderPass;
		info.subpass = 0;

		DeviceImplVulkan* device = queue->device;
		Verify("create graphics pipelines", vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &info, device->allocator, &instance.pipeline));

		return BinaryInsert(stateInstances, MakeKeyValue(std::move(key), std::move(instance)))->second;
	}

	 void Delete(QueueImplVulkan& queue) {
		Clear(queue);
		delete this;
	}

	static VkSamplerAddressMode ConvertAddressMode(uint32_t addressMode) {
		switch (addressMode) {
		case IRender::Resource::TextureDescription::REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case IRender::Resource::TextureDescription::CLAMP:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case IRender::Resource::TextureDescription::MIRROR_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case IRender::Resource::TextureDescription::MIRROR_CLAMP:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	void Upload(QueueImplVulkan& queue) {
		IRender::Resource::ShaderDescription& pass = description;
		DeviceImplVulkan* device = queue.device;

		std::vector<IShader*> shaders[Resource::ShaderDescription::END];
		String common;
		for (size_t i = 0; i < pass.entries.size(); i++) {
			const std::pair<Resource::ShaderDescription::Stage, IShader*>& component = pass.entries[i];

			if (component.first == Resource::ShaderDescription::GLOBAL) {
				common += component.second->GetShaderText();
			} else {
				shaders[component.first].emplace_back(component.second);
			}
		}

		textureBindings.clear();
		uniformBufferBindings.clear();
		storageBufferBindings.clear();

		uint32_t bindingIndex = 0;
		for (size_t k = 0; k < Resource::ShaderDescription::END; k++) {
			std::vector<IShader*>& pieces = shaders[k];
			if (pieces.empty()) continue;

			String body = "void main(void) {\n";
			String head = "";
			uint32_t inputIndex = 0, outputIndex = 0;
			VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			switch (k) {
			case IRender::Resource::ShaderDescription::VERTEX:
				stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_CONTROL:
				stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_EVALUATION:
				stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				break;
			case IRender::Resource::ShaderDescription::GEOMETRY:
				stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
				break;
			case IRender::Resource::ShaderDescription::FRAGMENT:
				stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case IRender::Resource::ShaderDescription::COMPUTE:
				stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}

			String predefines;
			for (size_t n = 0; n < pieces.size(); n++) {
				IShader* shader = pieces[n];
				// Generate declaration
				GLSLShaderGenerator declaration((Resource::ShaderDescription::Stage)k, inputIndex, outputIndex, bindingIndex);
				declaration.forceLayout = true;
				declaration.vulkanNDC = true;

				(*shader)(declaration);
				declaration.Complete();
				predefines += shader->GetPredefines();
				PreConstExpr preConstExpr;
				preConstExpr.variables = std::move(declaration.constants);
				body += declaration.initialization + preConstExpr(GLSLShaderGenerator::FormatCode(shader->GetShaderText())) + declaration.finalization + "\n";

				for (size_t i = 0; i < declaration.structures.size(); i++) {
					head += declaration.mapStructureDefinition[declaration.structures[i]];
				}

				head += declaration.declaration;

				for (size_t j = 0; j < declaration.bufferBindings.size(); j++) {
					GLSLShaderGenerator::Binding<IShader::BindBuffer>& item = declaration.bufferBindings[j];
					const IShader::BindBuffer* bindBuffer = item.pointer;
					bufferDescriptions.emplace_back(bindBuffer->description);
					if (bindBuffer->description.state.usage == IRender::Resource::BufferDescription::UNIFORM || bindBuffer->description.state.usage == IRender::Resource::BufferDescription::STORAGE) {
						VkDescriptorSetLayoutBinding binding = {};
						binding.descriptorCount = 1;
						binding.descriptorType = bindBuffer->description.state.usage == IRender::Resource::BufferDescription::UNIFORM ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						binding.stageFlags = stageFlags;
						binding.pImmutableSamplers = nullptr;
						binding.binding = item.binding;

						if (bindBuffer->description.state.usage == IRender::Resource::BufferDescription::UNIFORM) {
							uniformBufferBindings.emplace_back(std::move(binding));
						} else {
							storageBufferBindings.emplace_back(std::move(binding));
						}
					}
				}

				for (size_t m = 0; m < declaration.textureBindings.size(); m++) {
					VkDescriptorSetLayoutBinding binding;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.descriptorCount = 1;
					binding.stageFlags = stageFlags;
					binding.binding = declaration.textureBindings[m].binding;

					binding.pImmutableSamplers = nullptr;
					textureBindings.emplace_back(binding);
				}
			}

			body += "\n}\n"; // make a call to our function

			String fullShader = GLSLShaderGenerator::GetFrameCode() + predefines + common + head + body;
			// Vulkan only support SPIRV shader
			String byteCode = SPIRVCompiler::Compile((IRender::Resource::ShaderDescription::Stage)k, fullShader);
			if (byteCode.empty()) {
				fprintf(stderr, "Compile shader failed!");
				assert(false);
			}

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = byteCode.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.c_str());

			Verify("create shader module", vkCreateShaderModule(device->device, &createInfo, device->allocator, &shaderModules[k]));

			if (pass.compileCallback) {
				pass.compileCallback(this, pass, (IRender::Resource::ShaderDescription::Stage)k, "", fullShader);
			}

			VkPipelineShaderStageCreateInfo shaderStageInfo = {};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStageInfo.module = shaderModules[k];
			shaderStageInfo.pName = "main";

			switch (k) {
			case IRender::Resource::ShaderDescription::VERTEX:
				shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_CONTROL:
				shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_EVALUATION:
				shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				break;
			case IRender::Resource::ShaderDescription::GEOMETRY:
				shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				break;
			case IRender::Resource::ShaderDescription::FRAGMENT:
				shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case IRender::Resource::ShaderDescription::COMPUTE:
				shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}

			shaderStageCreateInfos.emplace_back(std::move(shaderStageInfo));
		}

		// TODO: push constant range
		// VkPushConstantRange 
		// Format descriptor layout sets

		uint32_t layoutCount = 0;
		VkDescriptorSetLayout layouts[3] = {};
		if (!textureBindings.empty()) {
			VkDescriptorSetLayoutCreateInfo textureInfo = {};
			textureInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			textureInfo.bindingCount = (uint32_t)textureBindings.size();
			textureInfo.pBindings = &textureBindings[0];
			Verify("create descriptor set layout", vkCreateDescriptorSetLayout(device->device, &textureInfo, device->allocator, &descriptorSetLayoutTexture));
			layouts[layoutCount++] = descriptorSetLayoutTexture;
		}

		if (!uniformBufferBindings.empty()) {
			VkDescriptorSetLayoutCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			bufferInfo.bindingCount = (uint32_t)uniformBufferBindings.size();
			bufferInfo.pBindings = &uniformBufferBindings[0];
			Verify("create descriptor set layout", vkCreateDescriptorSetLayout(device->device, &bufferInfo, device->allocator, &descriptorSetLayoutUniformBuffer));
			layouts[layoutCount++] = descriptorSetLayoutUniformBuffer;
		}

		if (!storageBufferBindings.empty()) {
			VkDescriptorSetLayoutCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			bufferInfo.bindingCount = (uint32_t)storageBufferBindings.size();
			bufferInfo.pBindings = &storageBufferBindings[0];
			Verify("create descriptor set layout", vkCreateDescriptorSetLayout(device->device, &bufferInfo, device->allocator, &descriptorSetLayoutStorageBuffer));
			layouts[layoutCount++] = descriptorSetLayoutStorageBuffer;
		}
	
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = layoutCount;
		layoutInfo.pSetLayouts = layouts;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;

		Verify("create pipeline layout", vkCreatePipelineLayout(device->device, &layoutInfo, device->allocator, &pipelineLayout));
	}

	VkDescriptorSetLayout descriptorSetLayoutTexture;
	VkDescriptorSetLayout descriptorSetLayoutUniformBuffer;
	VkDescriptorSetLayout descriptorSetLayoutStorageBuffer;
	
	std::vector<VkDescriptorSetLayoutBinding> textureBindings;
	std::vector<VkDescriptorSetLayoutBinding> uniformBufferBindings;
	std::vector<VkDescriptorSetLayoutBinding> storageBufferBindings;

	VkPipelineLayout pipelineLayout;
	VkShaderModule shaderModules[IRender::Resource::ShaderDescription::Stage::END];

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
	std::vector<IRender::Resource::BufferDescription> bufferDescriptions;
	std::vector<KeyValue<PipelineKey, PipelineInstance> > stateInstances;
};

template <>
class ResourceImplVulkan<IRender::Resource::TransferDescription> final
	: public ResourceBaseVulkanDesc<IRender::Resource::TransferDescription, IRender::Resource::RESOURCE_TRANSFER, ResourceImplVulkan<IRender::Resource::TransferDescription> > {
public:
	ResourceImplVulkan() {}

	void Execute(QueueImplVulkan& queue) {
		ResourceBaseImplVulkan* sourceImpl = static_cast<ResourceBaseImplVulkan*>(description.source);
		assert(sourceImpl != nullptr);

		// now only support rt -> rt copy by now
		if (sourceImpl->dispatchTable->type == IRender::Resource::RESOURCE_RENDERTARGET) {
			ResourceBaseImplVulkan* targetImpl = static_cast<ResourceBaseImplVulkan*>(description.target);
			ResourceImplVulkan<IRender::Resource::RenderTargetDescription>* sourceRenderTarget = static_cast<ResourceImplVulkan<IRender::Resource::RenderTargetDescription>*>(sourceImpl);
			ResourceImplVulkan<IRender::Resource::TextureDescription>* sourceImageResource =
				static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(description.copyColor ?
					sourceRenderTarget->description.colorStorages[description.sourceIndex].resource : description.copyDepth ?
					sourceRenderTarget->description.depthStorage.resource : description.copyStencil ?
					sourceRenderTarget->description.stencilStorage.resource : nullptr);
			assert(sourceImageResource != nullptr);
			VkImage sourceImage = sourceImageResource->image;
			VkImage targetImage = VK_NULL_HANDLE;
			VkImageLayout targetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// copy to back buffer?
			Int2Pair from = description.sourceRegion;
			Int2Pair to = description.targetRegion;
			if (targetImpl == nullptr) {
				targetImage = queue.device->GetCurrentFrame().backBufferImage;
				targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				
				if (to.second.x() == 0) {
					to.second.x() = queue.device->resolution.x();
				}

				if (to.second.y() == 0) {
					to.second.y() = queue.device->resolution.y();
				}

				VkImageMemoryBarrier targetBarrier;
				targetBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				targetBarrier.pNext = nullptr;
				targetBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				targetBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				targetBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				targetBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				targetBarrier.image = targetImage;
				targetBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				targetBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				targetBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				targetBarrier.subresourceRange.baseMipLevel = 0;
				targetBarrier.subresourceRange.levelCount = 1;
				targetBarrier.subresourceRange.baseArrayLayer = 0;
				targetBarrier.subresourceRange.layerCount = 1;

				vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &targetBarrier);
				queue.currentCommandBuffer.commandCount++;
			} else if (targetImpl->dispatchTable->type == IRender::Resource::RESOURCE_RENDERTARGET) {
				ResourceImplVulkan<IRender::Resource::RenderTargetDescription>* targetRenderTarget = static_cast<ResourceImplVulkan<IRender::Resource::RenderTargetDescription>*>(targetImpl);
				ResourceImplVulkan<IRender::Resource::TextureDescription>* targetImageResource =
					static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(description.copyColor ?
						targetRenderTarget->description.colorStorages[description.targetIndex].resource : description.copyDepth ?
						targetRenderTarget->description.depthStorage.resource : description.copyStencil ?
						targetRenderTarget->description.stencilStorage.resource : nullptr);
				targetImage = targetImageResource->image;
			} else {
				return; // not supported
			}

			VkImageMemoryBarrier sourceBarrier;
			sourceBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			sourceBarrier.pNext = nullptr;
			sourceBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			sourceBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceBarrier.image = sourceImage;
			sourceBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			sourceBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			sourceBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			sourceBarrier.subresourceRange.baseMipLevel = 0;
			sourceBarrier.subresourceRange.levelCount = 1;
			sourceBarrier.subresourceRange.baseArrayLayer = 0;
			sourceBarrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &sourceBarrier);
			queue.currentCommandBuffer.commandCount++;

			VkImageBlit region = {};
			region.srcSubresource.aspectMask = description.copyColor ? VK_IMAGE_ASPECT_COLOR_BIT : description.copyDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_STENCIL_BIT;
			region.srcSubresource.mipLevel = description.sourceMipLevel;
			region.srcSubresource.baseArrayLayer = description.sourceBaseArrayLayer;
			region.srcSubresource.layerCount = description.sourceLayerCount;

			region.dstSubresource.aspectMask = description.copyColor ? VK_IMAGE_ASPECT_COLOR_BIT : description.copyDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_STENCIL_BIT;
			region.dstSubresource.mipLevel = description.sourceMipLevel;
			region.dstSubresource.baseArrayLayer = description.sourceBaseArrayLayer;
			region.dstSubresource.layerCount = description.sourceLayerCount;

			region.srcOffsets[0] = { from.first.x(), from.first.y(), 0 };
			region.srcOffsets[1] = { from.second.x(), from.second.y(), 1 };
			region.dstOffsets[0] = { to.first.x(), to.first.y(), 0 };
			region.dstOffsets[1] = { to.second.x(), to.second.y(), 1 };

			vkCmdBlitImage(queue.currentCommandBuffer.buffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetImage, targetLayout, 1, &region, description.sample == IRender::Resource::TextureDescription::POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);
			queue.currentCommandBuffer.commandCount++;

			if (targetImpl == nullptr) {
				VkImageMemoryBarrier targetBarrier;
				targetBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				targetBarrier.pNext = nullptr;
				targetBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
				targetBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				targetBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				targetBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				targetBarrier.image = targetImage;
				targetBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				targetBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				targetBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				targetBarrier.subresourceRange.baseMipLevel = 0;
				targetBarrier.subresourceRange.levelCount = 1;
				targetBarrier.subresourceRange.baseArrayLayer = 0;
				targetBarrier.subresourceRange.layerCount = 1;

				vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &targetBarrier);
				queue.currentCommandBuffer.commandCount++;
			}

			VkImageMemoryBarrier sourceRevertBarrier;
			sourceRevertBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			sourceRevertBarrier.pNext = nullptr;
			sourceRevertBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			sourceRevertBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			sourceRevertBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceRevertBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			sourceRevertBarrier.image = sourceImage;
			sourceRevertBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			sourceRevertBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			sourceRevertBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			sourceRevertBarrier.subresourceRange.baseMipLevel = 0;
			sourceRevertBarrier.subresourceRange.levelCount = 1;
			sourceRevertBarrier.subresourceRange.baseArrayLayer = 0;
			sourceRevertBarrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(queue.currentCommandBuffer.buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &sourceRevertBarrier);
			queue.currentCommandBuffer.commandCount++;
		}
	}

	void Delete(QueueImplVulkan& queue) {
		delete this;
	}
};

// Resource
IRender::Resource* ZRenderVulkan::CreateResource(Device* device, Resource::Type resourceType, Queue* optionalHostQueue) {
	switch (resourceType)
	{
	case Resource::RESOURCE_UNKNOWN:
		assert(false);
		return nullptr;
	case Resource::RESOURCE_TEXTURE:
		return new ResourceImplVulkan<Resource::TextureDescription>();
	case Resource::RESOURCE_BUFFER:
		return new ResourceImplVulkan<Resource::BufferDescription>();
	case Resource::RESOURCE_SHADER:
		return new ResourceImplVulkan<Resource::ShaderDescription>();
	case Resource::RESOURCE_RENDERSTATE:
		return new ResourceImplVulkan<Resource::RenderStateDescription>();
	case Resource::RESOURCE_RENDERTARGET:
		return new ResourceImplVulkan<Resource::RenderTargetDescription>();
	case Resource::RESOURCE_DRAWCALL:
		return new ResourceImplVulkan<Resource::DrawCallDescription>();
	case Resource::RESOURCE_QUICK_DRAWCALL:
		return new ResourceImplVulkan<Resource::QuickDrawCallDescription>();
	case Resource::RESOURCE_EVENT:
		return new ResourceImplVulkan<Resource::EventDescription>();
	case Resource::RESOURCE_TRANSFER:
		return new ResourceImplVulkan<Resource::TransferDescription>();
	}

	assert(false);
	return nullptr;
}

const void* ZRenderVulkan::GetResourceDeviceHandle(IRender::Resource* resource) {
	assert(resource != nullptr);
	ResourceBaseImplVulkan* base = static_cast<ResourceBaseImplVulkan*>(resource);
	ResourceBaseImplVulkan::GetRawHandle getHandle = base->dispatchTable->getHandle;
	return (base->*getHandle)();
}

void ZRenderVulkan::SetupBarrier(Queue* queue, Barrier* barrier) {
	// memory barrier?
	QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queue);
	ResourceBaseImplVulkan* resource = static_cast<ResourceBaseImplVulkan*>(barrier->resource);

	QueueImplVulkan::LockGuard guard(*q);
	if (resource == nullptr) {
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = (VkAccessFlags)barrier->srcAccessMask; // totally mapped
		memoryBarrier.dstAccessMask = (VkAccessFlags)barrier->dstAccessMask; // totally mapped

		vkCmdPipelineBarrier(q->currentCommandBuffer.buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		q->currentCommandBuffer.commandCount++;
	} else {
		Resource::Type resourceType = resource->dispatchTable->type;
		if (resourceType == Resource::RESOURCE_BUFFER) {
			ResourceImplVulkan<Resource::BufferDescription>* impl = static_cast<ResourceImplVulkan<Resource::BufferDescription>*>(resource);
			VkBufferMemoryBarrier memoryBarrier;
			memoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			memoryBarrier.pNext = nullptr;
			memoryBarrier.srcAccessMask = (VkAccessFlags)barrier->srcAccessMask; // totally mapped
			memoryBarrier.dstAccessMask = (VkAccessFlags)barrier->dstAccessMask; // totally mapped
			memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memoryBarrier.buffer = impl->buffer;
			memoryBarrier.offset = barrier->offset;
			memoryBarrier.size = barrier->size == 0 ? impl->memorySize : barrier->size;

			vkCmdPipelineBarrier(q->currentCommandBuffer.buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 0, nullptr, 1, &memoryBarrier, 0, nullptr);
			q->currentCommandBuffer.commandCount++;
		} else if (resourceType == Resource::RESOURCE_TEXTURE) {
			ResourceImplVulkan<Resource::TextureDescription>* impl = static_cast<ResourceImplVulkan<Resource::TextureDescription>*>(resource);
			VkImageMemoryBarrier memoryBarrier;
			memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memoryBarrier.pNext = nullptr;
			memoryBarrier.srcAccessMask = (VkAccessFlags)barrier->srcAccessMask; // totally mapped
			memoryBarrier.dstAccessMask = (VkAccessFlags)barrier->dstAccessMask; // totally mapped
			memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memoryBarrier.image = impl->image;
			memoryBarrier.oldLayout = (VkImageLayout)barrier->oldLayout;
			memoryBarrier.newLayout = (VkImageLayout)barrier->newLayout;
			memoryBarrier.subresourceRange.aspectMask = barrier->aspectMask;
			memoryBarrier.subresourceRange.baseMipLevel = barrier->baseMipLevel;
			memoryBarrier.subresourceRange.levelCount = barrier->levelCount;
			memoryBarrier.subresourceRange.baseArrayLayer = barrier->baseArrayLayer;
			memoryBarrier.subresourceRange.layerCount = barrier->layerCount;

			vkCmdPipelineBarrier(q->currentCommandBuffer.buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
			q->currentCommandBuffer.commandCount++;
		} else {
			assert(false);
		}
	}
}

IRender::Resource::Description* ZRenderVulkan::MapResource(Queue* queue, Resource* resource, uint32_t mapFlags) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	// printf("Map resource: %p on %p\n", resource, queue);

	ResourceBaseImplVulkan* impl = static_cast<ResourceBaseImplVulkan*>(resource);

	if (mapFlags & IRender::MAP_DATA_EXCHANGE) {
		QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queue);
		assert(q != nullptr);

		// request to download?
		IRender::Resource::Description* description = impl->downloadDescription.load(std::memory_order_acquire);
		QueueImplVulkan::LockGuard guard(*q);
		if (description != nullptr) {
			ResourceBaseImplVulkan::Action p = impl->dispatchTable->actionSynchronizeDownload;
			(impl->*p)(*q);
		} else {
			ResourceBaseImplVulkan::Action p = impl->dispatchTable->actionDownload;
			(impl->*p)(*q);
		}

		return description;
	} else {
		ResourceBaseImplVulkan::DescriptionAddress p = impl->dispatchTable->descriptionAddress;
		return &(impl->*p);
	}
}

void ZRenderVulkan::UnmapResource(Queue* queue, Resource* resource, uint32_t mapFlags) {
	// printf("Unmap resource: %p on %p\n", resource, queue);
	assert(queue != nullptr);
	assert(resource != nullptr);
	ResourceBaseImplVulkan* impl = static_cast<ResourceBaseImplVulkan*>(resource);

	if (mapFlags & IRender::MAP_DATA_EXCHANGE) {
		QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queue);
		assert(q != nullptr);
		// request to upload?
		ResourceBaseImplVulkan::Action p = impl->dispatchTable->actionUpload;
		QueueImplVulkan::LockGuard guard(*q);
		(impl->*p)(*q);
	}
}

void ZRenderVulkan::ExecuteResource(Queue* queue, Resource* resource) {
	assert(queue != nullptr);
	assert(resource != nullptr);
	QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queue);
	ResourceBaseImplVulkan* impl = static_cast<ResourceBaseImplVulkan*>(resource);
	ResourceBaseImplVulkan::Action p = impl->dispatchTable->actionExecute;
	// printf("Execute resource: %p on %p\n", resource, queue);

	// TODO: Execute Queue as resource (merge secondary queue)
	QueueImplVulkan::LockGuard guard(*q);
	(impl->*p)(*q);
}

void ZRenderVulkan::SetResourceNote(Resource* resource, const String& note) {
#ifdef _DEBUG
	ResourceBaseImplVulkan* base = static_cast<ResourceBaseImplVulkan*>(resource);
	base->note = note;
#endif
}

void ZRenderVulkan::DeleteResource(Queue* queue, Resource* resource) {
	QueueImplVulkan* q = static_cast<QueueImplVulkan*>(queue);
	// printf("Delete resource: %p on %p\n", resource, queue);
	ResourceBaseImplVulkan* impl = static_cast<ResourceBaseImplVulkan*>(resource);
	ResourceBaseImplVulkan::Action p = impl->dispatchTable->actionDelete;
	QueueImplVulkan::LockGuard guard(*q);
	(impl->*p)(*q);
}

static inline VkBuffer GetBuffer(IRender::Resource::DrawCallDescription::BufferRange& range) {
	ResourceImplVulkan<IRender::Resource::BufferDescription>* bufferResource = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(range.buffer);
	return bufferResource->buffer;
}

void VulkanDrawCallResourceBase::UploadImpl(QueueImplVulkan& queue, IRender::Resource* shaderResource, IRender::Resource::DrawCallDescription::BufferRange* bufferResources, uint32_t bufferCount, IRender::Resource** textureResources, uint32_t textureCount) {
	// Compute signature
	std::vector<DrawSignatureItem> signatureBuffer;
	ResourceImplVulkan<IRender::Resource::ShaderDescription>* shader = static_cast<ResourceImplVulkan<IRender::Resource::ShaderDescription>*>(shaderResource);

	DeviceImplVulkan* device = queue.device;
	if (descriptorSetTexture == VK_NULL_HANDLE && shader->descriptorSetLayoutTexture != VK_NULL_HANDLE) {
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER];
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &shader->descriptorSetLayoutTexture;

		TSpinLockGuard<size_t> guard(device->descriptorCritical);
		vkAllocateDescriptorSets(device->device, &descriptorSetAllocateInfo, &descriptorSetTexture);
	}

	if (descriptorSetUniformBuffer == VK_NULL_HANDLE && shader->descriptorSetLayoutUniformBuffer != VK_NULL_HANDLE) {
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER];
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &shader->descriptorSetLayoutUniformBuffer;

		TSpinLockGuard<size_t> guard(device->descriptorCritical);
		vkAllocateDescriptorSets(device->device, &descriptorSetAllocateInfo, &descriptorSetUniformBuffer);
	}

	if (descriptorSetStorageBuffer == VK_NULL_HANDLE && shader->descriptorSetLayoutStorageBuffer != VK_NULL_HANDLE) {
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER];
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &shader->descriptorSetLayoutStorageBuffer;

		TSpinLockGuard<size_t> guard(device->descriptorCritical);
		vkAllocateDescriptorSets(device->device, &descriptorSetAllocateInfo, &descriptorSetStorageBuffer);
	}

	// upload bindings
	std::vector<VkDescriptorBufferInfo> uniformBufferInfos;
	std::vector<VkDescriptorBufferInfo> storageBufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;

	for (uint32_t i = 0; i < bufferCount; i++) {
		const IRender::Resource::DrawCallDescription::BufferRange& bufferRange = bufferResources[i];
		assert(bufferRange.buffer != nullptr);

		ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
		IRender::Resource::BufferDescription& desc = buffer->description;
		if (desc.state.usage == IRender::Resource::BufferDescription::VERTEX || desc.state.usage == IRender::Resource::BufferDescription::INSTANCED) {
			assert(desc.state.component < 32);

			DrawSignatureItem item;
			item.format = desc.state.format;
			item.component = desc.state.component;

			signatureBuffer.emplace_back(item);
		} else if (desc.state.usage == IRender::Resource::BufferDescription::UNIFORM || desc.state.usage == IRender::Resource::BufferDescription::STORAGE) {
			VkDescriptorBufferInfo info = {};
			assert(buffer->buffer != VK_NULL_HANDLE);
			info.buffer = buffer->buffer;
			info.offset = bufferRange.offset;
			info.range = bufferRange.length == 0 ? buffer->memorySize : bufferRange.length;

			if (desc.state.usage == IRender::Resource::BufferDescription::UNIFORM) {
				uniformBufferInfos.emplace_back(std::move(info));
			} else {
				storageBufferInfos.emplace_back(std::move(info));
			}
		} else {
			assert(false); // not supported by now
		}
	}

	for (uint32_t i = 0; i < textureCount; i++) {
		ResourceImplVulkan<IRender::Resource::TextureDescription>* texture = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(textureResources[i]);

		VkDescriptorImageInfo info = {};
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = texture->imageViewDepthAspect == VK_NULL_HANDLE ? texture->imageView : texture->imageViewDepthAspect;
		info.sampler = texture->imageSampler;

		imageInfos.emplace_back(std::move(info));
	}

	assert(imageInfos.size() == shader->textureBindings.size());
	for (uint32_t n = 0; n < imageInfos.size(); n++) {
		VkWriteDescriptorSet desc = {};
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = descriptorSetTexture;
		desc.dstBinding = shader->textureBindings[n].binding;
		desc.descriptorCount = 1;
		desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc.pImageInfo = &imageInfos[n];

		vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
	}

	assert(uniformBufferInfos.size() == shader->uniformBufferBindings.size());
	for (uint32_t m = 0; m < uniformBufferInfos.size(); m++) {
		VkWriteDescriptorSet desc = {};
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = descriptorSetUniformBuffer;
		desc.dstBinding = shader->uniformBufferBindings[m].binding;
		desc.descriptorCount = verify_cast<uint32_t>(uniformBufferInfos.size());
		desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc.pBufferInfo = &uniformBufferInfos[m];
		vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
	}

	assert(storageBufferInfos.size() == shader->storageBufferBindings.size());
	for (uint32_t t = 0; t < storageBufferInfos.size(); t++) {
		VkWriteDescriptorSet desc = {};
		desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		desc.dstSet = descriptorSetStorageBuffer;
		desc.dstBinding = shader->storageBufferBindings[t].binding;
		desc.descriptorCount = verify_cast<uint32_t>(storageBufferInfos.size());
		desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		desc.pBufferInfo = &storageBufferInfos[t];
		vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
	}

	drawSignature.Resize(signatureBuffer.size() * sizeof(DrawSignatureItem));
	if (!signatureBuffer.empty()) {
		memcpy(drawSignature.GetData(), &signatureBuffer[0], signatureBuffer.size() * sizeof(DrawSignatureItem));
	}
}

void ResourceImplVulkan<IRender::Resource::DrawCallDescription>::Upload(QueueImplVulkan& queue) {
	UploadImpl(queue, description.shaderResource, description.bufferResources.empty() ? nullptr : &description.bufferResources[0], verify_cast<uint32_t>(description.bufferResources.size()), description.textureResources.empty() ? nullptr : &description.textureResources[0], verify_cast<uint32_t>(description.textureResources.size()));
}

void ResourceImplVulkan<IRender::Resource::QuickDrawCallDescription>::Upload(QueueImplVulkan& queue) {
	UploadImpl(queue, description.GetShader(), description.GetBuffers(), description.bufferCount, description.GetTextures(), description.textureCount);
}

void VulkanDrawCallResourceBase::ExecuteImpl(QueueImplVulkan& queue, IRender::Resource* shaderResource, IRender::Resource::DrawCallDescription::BufferRange* indexBufferResource, IRender::Resource::DrawCallDescription::BufferRange* bufferResources, uint32_t bufferCount, IRender::Resource** textureResources, uint32_t textureCount, const UInt3& instanceCounts) {
	ResourceImplVulkan<IRender::Resource::ShaderDescription>* shader = static_cast<ResourceImplVulkan<IRender::Resource::ShaderDescription>*>(shaderResource);

	PipelineInstance& pipelineInstance = shader->QueryInstance(&queue, drawSignature, bufferResources, bufferCount);
	VkCommandBuffer commandBuffer = queue.currentCommandBuffer.buffer;
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInstance.pipeline);
	queue.currentCommandBuffer.commandCount++;
	VkDescriptorSet descriptors[3];
	uint32_t i = 0;
	if (descriptorSetTexture != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetTexture;
	}

	if (descriptorSetUniformBuffer != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetUniformBuffer;
	}

	if (descriptorSetStorageBuffer != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetStorageBuffer;
	}

	if (i != 0) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shader->pipelineLayout, 0, 1, descriptors, 0, nullptr);
		queue.currentCommandBuffer.commandCount++;
	}

	ResourceImplVulkan<IRender::Resource::BufferDescription>* indexBuffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(indexBufferResource->buffer);

	auto res = GetFormatSize(indexBuffer->description.state.format);
	VkIndexType indexType = res.second;
	uint32_t indexElementSize = res.first;
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, indexBufferResource->offset, indexType);
	queue.currentCommandBuffer.commandCount++;
	
	std::vector<VkBuffer> vertexBuffers;
	std::vector<VkDeviceSize> vertexOffsets;
	for (uint32_t k = 0; k < bufferCount; k++) {
		ResourceImplVulkan<IRender::Resource::BufferDescription>* p = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferResources[k].buffer);
		if (p->description.state.usage == IRender::Resource::BufferDescription::VERTEX || p->description.state.usage == IRender::Resource::BufferDescription::INSTANCED) {
			vertexBuffers.emplace_back(p->buffer);
			vertexOffsets.emplace_back(bufferResources[k].offset);
		}
	}

	assert(!vertexBuffers.empty());
	vkCmdBindVertexBuffers(commandBuffer, 0, verify_cast<uint32_t>(vertexBuffers.size()), &vertexBuffers[0], &vertexOffsets[0]);
	queue.currentCommandBuffer.commandCount++;
	vkCmdDrawIndexed(commandBuffer, indexBufferResource->length / indexElementSize, instanceCounts.x(), 0, 0, 0);
	queue.currentCommandBuffer.commandCount++;
}

void ResourceImplVulkan<IRender::Resource::DrawCallDescription>::Execute(QueueImplVulkan& queue) {
	VulkanDrawCallResourceBase::ExecuteImpl(queue, description.shaderResource, &description.indexBufferResource, description.bufferResources.empty() ? nullptr : &description.bufferResources[0], verify_cast<uint32_t>(description.bufferResources.size()), description.textureResources.empty() ? nullptr : &description.textureResources[0], verify_cast<uint32_t>(description.textureResources.size()), description.instanceCounts);
}

void ResourceImplVulkan<IRender::Resource::QuickDrawCallDescription>::Execute(QueueImplVulkan& queue) {
	VulkanDrawCallResourceBase::ExecuteImpl(queue, description.GetShader(), description.GetIndexBuffer(), description.GetBuffers(), description.bufferCount, description.GetTextures(), description.textureCount, UInt3(description.instanceCount, 0, 0));
}

template <bool finalize>
bool QueueImplVulkan::DispatchEventsEx(TQueueList<ResourceBaseImplVulkan*>& events, TQueueList<ResourceBaseImplVulkan*>& nextEvents) {
	while (!events.Empty()) {
		ResourceImplVulkan<IRender::Resource::EventDescription>* ev = static_cast<ResourceImplVulkan<IRender::Resource::EventDescription>*>(events.Top());
		events.Pop();

		if (ev != nullptr) {
			if (vkGetEventStatus(device->device, ev->eventHandle) == VK_EVENT_SET) {
				if (ev->description.eventCallback) {
					ev->description.eventCallback(device->render, this, ev, true);
				}
			} else {
				if /*constexpr*/ (finalize) {
					if (ev->description.eventCallback) {
						ev->description.eventCallback(device->render, this, ev, false);  // failed!
					}
				} else {
					QueueImplVulkan::LockGuard guard(*this);
					nextEvents.Push(ev);
				}
			}
		} else {
			// Go next frame
			return true;
		}
	}

	return false;
}

template <bool finalize>
bool QueueImplVulkan::DispatchEvents() {
	bool recycled = DispatchEventsEx<finalize>(recycledEvents, newEvents);
	bool recycledNew = DispatchEventsEx<finalize>(newEvents, recycledEvents);
	return recycled || recycledNew;
}

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
	fprintf(stderr, "Vulkan debug: %d - %s\n", objectType, pMessage);
	assert(false);
	return VK_FALSE;
}
#endif

ZRenderVulkan::ZRenderVulkan(GLFWwindow* win) : window(win) {
	frameIndex.store(0, std::memory_order_release);
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "PaintsNow.Net";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PaintsNow";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	uint32_t extensionsCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledExtensionCount = extensionsCount;
	createInfo.ppEnabledExtensionNames = extensions;

	// Create Vulkan Instance
	VkAllocationCallbacks* allocator = nullptr; // TODO: customize allocators

#ifdef _DEBUG 
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;

	// Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
	std::vector<const char*> extensions_ext(extensionsCount + 1);
	memcpy(&extensions_ext[0], extensions, extensionsCount * sizeof(const char*));
	extensions_ext[extensionsCount] = "VK_EXT_debug_report";
	createInfo.enabledExtensionCount = extensionsCount + 1;
	createInfo.ppEnabledExtensionNames = &extensions_ext[0];

	Verify("create vulkan instance (debug)", vkCreateInstance(&createInfo, allocator, &instance));

	// Get the function pointer (required for any extensions)
	auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	assert(vkCreateDebugReportCallbackEXT != nullptr);

	// Setup the debug report callback
	VkDebugReportCallbackCreateInfoEXT debugReport = {};
	debugReport.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugReport.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	debugReport.pfnCallback = DebugReportCallback;
	debugReport.pUserData = nullptr;
	Verify("create debug report", vkCreateDebugReportCallbackEXT(instance, &debugReport, allocator, (VkDebugReportCallbackEXT*)&debugCallback));
#else
	// Create Vulkan Instance without any debug feature
	Verify("create vulkan instance (release)", vkCreateInstance(&createInfo, allocator, &instance));
#endif

	// Bind to window
	Verify("bind to window", glfwCreateWindowSurface(instance, window, allocator, (VkSurfaceKHR*)&surface));
}

ZRenderVulkan::~ZRenderVulkan() {
	VkAllocationCallbacks* allocator = nullptr; // TODO: customize allocators
#ifdef _DEBUG
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(instance, (VkDebugReportCallbackEXT)debugCallback, allocator);
#endif

	vkDestroySurfaceKHR(instance, (VkSurfaceKHR)surface, allocator);
	vkDestroyInstance(instance, nullptr);
}
