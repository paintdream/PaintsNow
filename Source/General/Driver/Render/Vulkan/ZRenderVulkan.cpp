#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "../../../../Core/Template/TPool.h"
#include "../OpenGL/GLSLShaderGenerator.h"
#include "SPIRVCompiler.h"
#include "ZRenderVulkan.h"
// #include <glslang/Public/ShaderLang.h>
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>

#define GLFW_INCLUDE_VULKAN
#include "../../Frame/GLFW/Core/include/GLFW/glfw3.h"

using namespace PaintsNow;

const int MIN_IMAGE_COUNT = 2;
const int MAX_DESCRIPTOR_SIZE = 1000;

static void Verify(const char* message, VkResult res) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "Unable to %s (debug).\n", message);
		exit(0);
	}
}

struct VulkanQueueImpl;
struct ResourceImplVulkanBase : public IRender::Resource {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) = 0;
	virtual void Delete(VulkanQueueImpl* queue) = 0;
	virtual void Execute(VulkanQueueImpl* queue) = 0;
	virtual void Download(VulkanQueueImpl* queue, IRender::Resource::Description* description) = 0;
	virtual Resource::Type GetResourceType() = 0;
	virtual const void* GetHandle() const { return nullptr; }
};

template <class T>
struct ResourceImplVulkanDesc : public ResourceImplVulkanBase {
	typedef ResourceImplVulkanDesc BaseClass;
	void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {
		cacheDescription = std::move(*static_cast<T*>(description));
	}

	void Download(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {}

	void Delete(VulkanQueueImpl* queue) override {}
	void Execute(VulkanQueueImpl* queue) override {}

	T cacheDescription;
};

struct FrameData {
	std::vector<VulkanQueueImpl*> activeQueues;
	std::vector<VulkanQueueImpl*> deletedQueues;

	VkImage backBufferImage;
	VkImageView backBufferView;
	VkFence fence;
	VkSemaphore acquireSemaphore;
	VkSemaphore releaseSemaphore;
};

struct Node {
	VkDescriptorPool pool;
	uint32_t freeCount;
	std::atomic<uint32_t> critical;
};

struct VulkanDeviceImpl;
struct VulkanQueueImpl;

struct DescriptorSetAllocator {
	std::pair<Node*, VkDescriptorSet> AllocateDescriptorSet(const VkDescriptorSetLayout& layout);
	void FreeDescriptorSet(const std::pair<Node*, VkDescriptorSet>& s);

	VulkanDeviceImpl* device;
	std::atomic<Node*> activeNode;
	VkDescriptorPoolSize sizeInfo;
};

struct VulkanDeviceImpl final : public IRender::Device {
	VulkanDeviceImpl(IRender& r, VkAllocationCallbacks* alloc, VkPhysicalDevice phy, VkDevice dev, uint32_t family, VkQueue q) : render(r), allocator(alloc), physicalDevice(phy), device(dev), resolution(0, 0), queueFamily(family), queue(q), swapChain(VK_NULL_HANDLE), currentFrameIndex(0) {
		for (size_t k = 0; k < sizeof(descriptorAllocators) / sizeof(descriptorAllocators[0]); k++) {
			DescriptorSetAllocator& alloc = descriptorAllocators[k];
			alloc.device = this;
			alloc.activeNode = nullptr;
			alloc.sizeInfo.descriptorCount = 256;
			alloc.sizeInfo.type = (VkDescriptorType)k;
		}

		critical.store(0, std::memory_order_release);
	}

	void DestroyAllFrames(VkAllocationCallbacks* allocator) {
		for (size_t i = 0; i < frames.size(); i++) {
			FrameData& frameData = frames[i];
			CleanupFrame(frameData);

			vkDestroySemaphore(device, frameData.acquireSemaphore, allocator);
			vkDestroySemaphore(device, frameData.releaseSemaphore, allocator);
			// vkDestroyImage(device, frameData.backBufferImage, allocator); // backBufferImage is auto-released in destroying swap chain
			vkDestroyImageView(device, frameData.backBufferView, allocator);
			vkDestroyFence(device, frameData.fence, allocator);
		}

		frames.clear();
	}

	~VulkanDeviceImpl() {
		DestroyAllFrames(allocator);

		vkDestroySwapchainKHR(device, swapChain, allocator);
		vkDestroyDevice(device, allocator);

		for (size_t k = 0; k < sizeof(descriptorAllocators) / sizeof(descriptorAllocators[0]); k++) {
			Node* p = descriptorAllocators[k].activeNode.load(std::memory_order_relaxed);
			if (p != nullptr) {
				delete p;
			}
		}
	}

	void CleanupFrame(FrameData& lastFrame);

	void NextFrame() {
		// rotate frames
		FrameData& lastFrame = frames[(currentFrameIndex + frames.size() - 1) % frames.size()];
		CleanupFrame(lastFrame);
	}

	std::atomic<uint32_t> critical;
	IRender& render;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkAllocationCallbacks* allocator;
	VkQueue queue;
	Int2 resolution;
	uint32_t queueFamily;
	uint32_t currentFrameIndex;
	VkSwapchainKHR swapChain;

	DescriptorSetAllocator descriptorAllocators[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1];
	std::vector<FrameData> frames;
	std::vector<VulkanQueueImpl*> deletedQueues;
};


std::pair<Node*, VkDescriptorSet> DescriptorSetAllocator::AllocateDescriptorSet(const VkDescriptorSetLayout& layout) {
	Node* p = activeNode.exchange(nullptr, std::memory_order_relaxed);

	if (p == nullptr) {
		p = new Node();

		p->freeCount = sizeInfo.descriptorCount;
		p->critical.store(0, std::memory_order_release);

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = sizeInfo.descriptorCount;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &sizeInfo;
		Verify("create descriptor pool", vkCreateDescriptorPool(device->device, &poolInfo, device->allocator, &p->pool));
	}

	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = p->pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &layout;

	SpinLock(p->critical);
	Verify("create descriptor set", vkAllocateDescriptorSets(device->device, &descriptorSetAllocateInfo, &descriptorSet));
	p->freeCount--;
	SpinUnLock(p->critical);

	if (p->freeCount != 0) {
		Node* expected = nullptr;
		activeNode.compare_exchange_strong(expected, p, std::memory_order_release);
	}

	return std::make_pair(p, descriptorSet);
}

void DescriptorSetAllocator::FreeDescriptorSet(const std::pair<Node*, VkDescriptorSet>& s) {
	Node* p = s.first;
	SpinLock(p->critical);
	vkFreeDescriptorSets(device->device, p->pool, 1, &s.second);
	++p->freeCount;
	SpinUnLock(p->critical);

	if (p->freeCount == sizeInfo.descriptorCount) {
		Node* t = activeNode.exchange(p, std::memory_order_release);
		if (t != nullptr && t != p && t->freeCount == sizeInfo.descriptorCount) {
			vkDestroyDescriptorPool(device->device, t->pool, device->allocator);
			delete t;
		}
	} else {
		Node* expected = nullptr;
		if (activeNode.load(std::memory_order_acquire) == nullptr) {
			activeNode.compare_exchange_strong(expected, p, std::memory_order_release);
		}
	}
}

struct VulkanQueueImpl final : public IRender::Queue {
	enum { MAX_POOLED_BUFFER_COUNT = 4 };
	struct BufferNode {
		VkCommandBuffer buffer;
		BufferNode* next;
	};

	VulkanQueueImpl(VulkanDeviceImpl* dev, VkCommandPool pool, uint32_t f) : device(dev), commandPool(pool), bufferPool(*this, MAX_POOLED_BUFFER_COUNT), renderTargetResource(nullptr), flag(f) {
		critical.store(0u, std::memory_order_relaxed);
		preparedCount.store(0, std::memory_order_release);

		currentCommandBuffer = bufferPool.Acquire();
		BeginCurrentCommandBuffer();
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

	~VulkanQueueImpl() {
		ClearFrameData();
		bufferPool.Clear();
		vkDestroyCommandPool(device->device, commandPool, device->allocator);
	}

	void DispatchEvents();

	void ClearFrameData() {
		DispatchEvents();

		while (!deletedResources.Empty()) {
			ResourceImplVulkanBase* res = deletedResources.Top();
			res->Delete(this);
			delete res;

			deletedResources.Pop();
		}

		while (!transientDataBuffers.Empty()) {
			VkBuffer buffer = transientDataBuffers.Top();
			vkDestroyBuffer(device->device, buffer, device->allocator);
			transientDataBuffers.Pop();
		}
	}

	BufferNode* allocate(size_t n) {
		assert(n == 1);
		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = commandPool;
		info.pNext = nullptr;

		VkCommandBuffer ret = nullptr;
		vkAllocateCommandBuffers(device->device, &info, &ret);

		BufferNode* node = new BufferNode();
		node->buffer = ret;
		node->next = nullptr;
		return node;
	}

	void construct(BufferNode* node) {}
	void destroy(BufferNode* node) {}

	void deallocate(BufferNode* node, size_t n) {
		assert(n == 1);
		vkFreeCommandBuffers(device->device, commandPool, 1, &node->buffer);
		delete node;
	}

	void EndCurrentCommandBuffer() {
		if (renderTargetResource != nullptr) {
			vkCmdEndRenderPass(currentCommandBuffer->buffer);
			renderTargetResource = nullptr;
		}

		vkEndCommandBuffer(currentCommandBuffer->buffer);
	}

	void BeginCurrentCommandBuffer() {
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = !(flag & IRender::SUBMIT_EXECUTE_REPEAT) ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
		vkBeginCommandBuffer(currentCommandBuffer->buffer, &info);
	}

	void FlushCommandBuffer() {
		EndCurrentCommandBuffer();

		preparedCommandBuffers.Push(currentCommandBuffer->buffer);
		preparedCount.fetch_add(1, std::memory_order_release);

		currentCommandBuffer = bufferPool.Acquire();
		vkResetCommandBuffer(currentCommandBuffer->buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

		BeginCurrentCommandBuffer();
	}

	void FlushFrame() {
		transientDataBuffers.Push((VkBuffer)VK_NULL_HANDLE);
		deletedResources.Push(nullptr);
	}

	VulkanDeviceImpl* device;
	VkCommandPool commandPool;
	BufferNode* currentCommandBuffer;
	uint32_t flag;
	std::atomic<uint32_t> critical;
	std::atomic<uint32_t> preparedCount;
	TPool<BufferNode, VulkanQueueImpl> bufferPool;

	IRender::Resource::RenderStateDescription renderStateDescription;
	ResourceImplVulkanBase* renderTargetResource;
	TQueueList<VkBuffer, 4> transientDataBuffers;
	TQueueList<VkCommandBuffer, 4> preparedCommandBuffers;
	TQueueList<ResourceImplVulkanBase*, 4> deletedResources;
	TQueueList<ResourceImplVulkanBase*, 4> newEvents;
	TQueueList<ResourceImplVulkanBase*, 4> pendingEvents;
};

void VulkanDeviceImpl::CleanupFrame(FrameData& lastFrame) {
	for (size_t k = 0; k < lastFrame.activeQueues.size(); k++) {
		VulkanQueueImpl* queue = lastFrame.activeQueues[k];
		queue->ClearFrameData();
	}

	lastFrame.activeQueues.clear();

	for (size_t n = 0; n < lastFrame.deletedQueues.size(); n++) {
		VulkanQueueImpl* queue = lastFrame.deletedQueues[n];
		delete queue;
	}

	lastFrame.deletedQueues.clear();
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

			VulkanDeviceImpl* impl = new VulkanDeviceImpl(*this, allocator, device, logicDevice, family, queue);

			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			SetDeviceResolution(impl, Int2(w, h));

			return impl;
		}
	}

	return nullptr;
}

void ZRenderVulkan::DeleteDevice(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	delete impl;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* dev, const Int2& resolution) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(dev);
	VkSwapchainKHR oldSwapChain = impl->swapChain;
	impl->swapChain = VK_NULL_HANDLE;
	VkAllocationCallbacks* allocator = impl->allocator;

	// reset device swap chain
	VkDevice device = impl->device;
	Verify("wait idle", vkDeviceWaitIdle(device));

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = (VkSurfaceKHR)surface;
	info.minImageCount = MIN_IMAGE_COUNT;
	info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = oldSwapChain;

	VkSurfaceCapabilitiesKHR cap;
	Verify("get physical device cap", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl->physicalDevice, (VkSurfaceKHR)surface, &cap));
	if (info.minImageCount < cap.minImageCount)
		info.minImageCount = cap.minImageCount;
	else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
		info.minImageCount = cap.maxImageCount;

	if (cap.currentExtent.width == 0xffffffff) {
		info.imageExtent.width = impl->resolution.x() = resolution.x();
		info.imageExtent.height = impl->resolution.y() = resolution.y();
	} else {
		info.imageExtent.width = impl->resolution.x() = cap.currentExtent.width;
		info.imageExtent.height = impl->resolution.y() = cap.currentExtent.height;
	}

	if (oldSwapChain)
		vkDestroySwapchainKHR(device, oldSwapChain, allocator);

	// Cleanup old frame data
	impl->DestroyAllFrames(allocator);

	Verify("create swap chain", vkCreateSwapchainKHR(device, &info, allocator, &impl->swapChain));
	uint32_t imageCount;
	Verify("get swapchain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, nullptr));

	std::vector<VkImage> images(imageCount);
	Verify("get swap chain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, &images[0]));

	impl->frames.resize(imageCount);
	VkImageViewCreateInfo viewinfo = {};
	viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewinfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	viewinfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewinfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewinfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewinfo.components.a = VK_COMPONENT_SWIZZLE_A;
	VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewinfo.subresourceRange = image_range;

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
		Verify("create semaphore (acquire)", vkCreateSemaphore(device, &seminfo, allocator, &frameData.acquireSemaphore));
		Verify("create semaphore (release)", vkCreateSemaphore(device, &seminfo, allocator, &frameData.releaseSemaphore));
	}

}

Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	return impl->resolution;
}

bool ZRenderVulkan::NextDeviceFrame(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	impl->NextFrame();
	return true;
}

IRender::Queue* ZRenderVulkan::CreateQueue(Device* dev, uint32_t flag) {
	VulkanDeviceImpl* device = static_cast<VulkanDeviceImpl*>(dev);
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = device->queueFamily;
	VkCommandPool commandPool;
	vkCreateCommandPool(device->device, &info, device->allocator, &commandPool);

	return new VulkanQueueImpl(device, commandPool, flag);
}

IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* q) {
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	return queue->device;
}

size_t ZRenderVulkan::GetProfile(Device* device, const String& feature) {
	return true; // by now all supported.
}

void ZRenderVulkan::SubmitQueues(Queue** queues, uint32_t count, SubmitOption option) {
	if (count != 0) {
		VulkanDeviceImpl* device = static_cast<VulkanQueueImpl*>(queues[0])->device;
		FrameData& frame = device->frames[device->currentFrameIndex];

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo info = {};

		std::vector<VkCommandBuffer> commandBuffers;
		commandBuffers.reserve(count * 2);

		for (uint32_t i = 0; i < count; i++) {
			VulkanQueueImpl* q = static_cast<VulkanQueueImpl*>(queues[i]);
			if (q->preparedCount.load(std::memory_order_acquire) > 0) {
				std::binary_insert(frame.activeQueues, q);

				while (!q->preparedCommandBuffers.Empty()) {
					VkCommandBuffer commandBuffer = q->preparedCommandBuffers.Top();
					assert(commandBuffer != VK_NULL_HANDLE);
					assert(q->preparedCount.load(std::memory_order_acquire) >= 0);

					if (option == IRender::SUBMIT_EXECUTE_REPEAT) {
						if (q->preparedCount.load(std::memory_order_acquire) == 1) {
							commandBuffers.push_back(commandBuffer);
							break;
						}
					}

					q->preparedCount.fetch_sub(1, std::memory_order_relaxed);
					q->preparedCommandBuffers.Pop();
					commandBuffers.push_back(commandBuffer); // TODO: add acquire-release

					if (option == IRender::SUBMIT_EXECUTE) {
						break;
					}
				}
			}
		}

		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &frame.acquireSemaphore;
		info.pWaitDstStageMask = &waitStage;
		info.commandBufferCount = verify_cast<uint32_t>(commandBuffers.size());
		info.pCommandBuffers = &commandBuffers[0];
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &frame.releaseSemaphore;

		Verify("queue submit", vkQueueSubmit(device->queue, 1, &info, frame.fence));
	}
}

void ZRenderVulkan::DeleteQueue(Queue* q) {
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	SpinLock(queue->device->critical);
	queue->device->deletedQueues.emplace_back(queue); // delayed delete
	SpinUnLock(queue->device->critical);

	/*
	vkDestroyCommandPool(queue->device->device, queue->commandPool, allocator);
	delete queue;
	*/
}

void ZRenderVulkan::FlushQueue(Queue* q) {
	// start new segment
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	queue->FlushCommandBuffer();
}

template <class T>
struct ResourceImplVulkan {};

static VkFormat TranslateFormat(uint32_t format, uint32_t layout) {
	static const VkFormat formats[IRender::Resource::TextureDescription::Layout::END][IRender::Resource::Description::Format::END] = {
		{ VK_FORMAT_R8_UNORM, VK_FORMAT_R16_UNORM, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT },
		{ VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_UINT },
		{ VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_UINT },
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_UINT },
		{ VK_FORMAT_UNDEFINED, VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_S8_UINT, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
	};

	return formats[layout][format];
}

template <>
struct ResourceImplVulkan<IRender::Resource::TextureDescription> final : public ResourceImplVulkanDesc<IRender::Resource::TextureDescription> {
	ResourceImplVulkan() : image(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE), memory(VK_NULL_HANDLE) {}
	~ResourceImplVulkan() {
	}

	const void* GetHandle() const override {
		return reinterpret_cast<const void*>(image);
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_TEXTURE;
	}

	void Clear(VulkanDeviceImpl* device) {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device->device, imageView, device->allocator);
			imageView = VK_NULL_HANDLE;
		}

		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(device->device, image, device->allocator);
			image = VK_NULL_HANDLE;
		}

		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device->device, memory, device->allocator);
		}
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		VulkanDeviceImpl* device = queue->device;
		IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(d);

		if (image == 0 || cacheDescription.dimension != desc.dimension || cacheDescription.state != desc.state) {
			Clear(device);

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

			info.format = TranslateFormat(desc.state.format, desc.state.layout);
			info.extent.width = desc.dimension.x();
			info.extent.height = desc.dimension.y();
			info.extent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.mipLevels = desc.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(desc.dimension.x(), desc.dimension.y())) + 1;
			info.arrayLayers = desc.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = desc.state.attachment ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			Verify("create image", vkCreateImage(device->device, &info, device->allocator, &image));

			VkMemoryRequirements req;
			vkGetImageMemoryRequirements(device->device, image, &req);

			VkMemoryAllocateInfo allocInfo;
			allocInfo.pNext = nullptr;
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			Verify("allocate texture memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &memory));
			Verify("bind image", vkBindImageMemory(device->device, image, memory, 0));

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.layerCount = 1;
			Verify("create image view", vkCreateImageView(device->device, &viewInfo, device->allocator, &imageView));
		}

		if (!desc.data.Empty()) {
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = desc.data.GetSize();
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer uploadBuffer;
			Verify("create buffer", vkCreateBuffer(device->device, &bufferInfo, device->allocator, &uploadBuffer));

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device->device, uploadBuffer, &req);
			// assert(((size_t)description.data.GetData() & ~req.alignment) == 0);
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			VkDeviceMemory bufferMemory;
			Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &bufferMemory));
			Verify("bind memory", vkBindBufferMemory(device->device, uploadBuffer, bufferMemory, 0));

			void* map = nullptr;
			Verify("map memory", vkMapMemory(device->device, bufferMemory, 0, desc.data.GetSize(), 0, &map));
			memcpy(map, desc.data.GetData(), desc.data.GetSize());
			VkMappedMemoryRange range = {};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = bufferMemory;
			range.size = desc.data.GetSize();
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
			copyBarrier.subresourceRange.levelCount = 1;
			copyBarrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(queue->currentCommandBuffer->buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);

			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = desc.dimension.x();
			region.imageExtent.height = desc.dimension.y();
			region.imageExtent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			vkCmdCopyBufferToImage(queue->currentCommandBuffer->buffer, uploadBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VkImageMemoryBarrier useBarrier = {};
			useBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.image = image;
			useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			useBarrier.subresourceRange.levelCount = 1;
			useBarrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(queue->currentCommandBuffer->buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &useBarrier);

			queue->transientDataBuffers.Push(uploadBuffer);
			desc.data.Clear();

			vkDestroyBuffer(device->device, uploadBuffer, device->allocator);
			vkFreeMemory(device->device, bufferMemory, device->allocator);
		}
	
		BaseClass::Upload(queue, d);
	}

	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
};

template <>
struct ResourceImplVulkan<IRender::Resource::BufferDescription> final : public ResourceImplVulkanDesc<IRender::Resource::BufferDescription> {
	ResourceImplVulkan() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE) {}
	~ResourceImplVulkan() {
		assert(buffer == VK_NULL_HANDLE);
		assert(memory == VK_NULL_HANDLE);
	}

	const void* GetHandle() const override {
		return reinterpret_cast<const void*>(buffer);
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_BUFFER;
	}

	void Clear(VulkanDeviceImpl* device) {
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device->device, buffer, device->allocator);
			buffer = VK_NULL_HANDLE;
		}

		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device->device, memory, device->allocator);
		}
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		VulkanDeviceImpl* device = queue->device;
		if (buffer != VK_NULL_HANDLE) {
			Clear(device);
		}

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(d);
		createInfo.size = desc.data.GetSize();
		switch (desc.usage) {
		case IRender::Resource::BufferDescription::INDEX:
			createInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::VERTEX:
			createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::INSTANCED:
			createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::UNIFORM:
			createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::STORAGE:
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		}

		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Verify("create buffer", vkCreateBuffer(device->device, &createInfo, device->allocator, &buffer));

		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device->device, buffer, &req);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &memory));
		Verify("bind memory", vkBindBufferMemory(device->device, buffer, memory, 0));

		void* map = nullptr;
		Verify("map memory", vkMapMemory(device->device, memory, 0, desc.data.GetSize(), 0, &map));
		memcpy(map, desc.data.GetData(), desc.data.GetSize());
		VkMappedMemoryRange range = {};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = memory;
		range.size = desc.data.GetSize();
		Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
		vkUnmapMemory(device->device, memory);

		desc.data.Clear();
		BaseClass::Upload(queue, d);
	}

	VkBuffer buffer;
	VkDeviceMemory memory;
};

template <>
struct ResourceImplVulkan<IRender::Resource::DrawCallDescription> final : public ResourceImplVulkanDesc<IRender::Resource::DrawCallDescription> {
	ResourceImplVulkan() {
		descriptorSetTexture.first = nullptr;
		descriptorSetTexture.second = VK_NULL_HANDLE;
		descriptorSetUniformBuffer.first = nullptr;
		descriptorSetUniformBuffer.second = VK_NULL_HANDLE;
		descriptorSetStorageBuffer.first = nullptr;
		descriptorSetStorageBuffer.second = VK_NULL_HANDLE;
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_DRAWCALL;
	}

	uint32_t GetVertexBufferCount() {
		return verify_cast<uint32_t>(signature.GetSize() / 3);
	}

	uint8_t GetVertexBufferBindingIndex(uint32_t index) {
		return signature.GetData()[index * 3];
	}

	void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override;
	void Execute(VulkanQueueImpl* queue) override;
	virtual void Delete(VulkanQueueImpl* queue) override {
		if (descriptorSetTexture.second != VK_NULL_HANDLE) {
			queue->device->descriptorAllocators[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER].FreeDescriptorSet(descriptorSetTexture);
		}

		if (descriptorSetUniformBuffer.second != VK_NULL_HANDLE) {
			queue->device->descriptorAllocators[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER].FreeDescriptorSet(descriptorSetUniformBuffer);
		}

		if (descriptorSetStorageBuffer.second != VK_NULL_HANDLE) {
			queue->device->descriptorAllocators[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER].FreeDescriptorSet(descriptorSetStorageBuffer);
		}
	}

	std::pair<Node*, VkDescriptorSet> descriptorSetTexture;
	std::pair<Node*, VkDescriptorSet> descriptorSetUniformBuffer;
	std::pair<Node*, VkDescriptorSet> descriptorSetStorageBuffer;

	Bytes signature;
};

template <>
struct ResourceImplVulkan<IRender::Resource::EventDescription> final : public ResourceImplVulkanDesc<IRender::Resource::EventDescription> {
	ResourceImplVulkan() : eventHandle(VK_NULL_HANDLE) {}
	void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		if (eventHandle == VK_NULL_HANDLE) {
			VkEventCreateInfo info;
			info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
			info.pNext = nullptr;
			info.flags = 0;
			vkCreateEvent(queue->device->device, &info, queue->device->allocator, &eventHandle);
		}

		EventDescription* ed = static_cast<EventDescription*>(d);
		if (ed->setState) {
			if (ed->newState) {
				vkSetEvent(queue->device->device, eventHandle);
			} else {
				vkResetEvent(queue->device->device, eventHandle);
			}
		}

		if (ed->setCallback) {
			cacheDescription.eventCallback = std::move(ed->eventCallback);
		}
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_EVENT;
	}

	void Download(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		EventDescription* ed = static_cast<EventDescription*>(d);
		if (ed->setState) {
			assert(eventHandle != VK_NULL_HANDLE);
			ed->newState = vkGetEventStatus(queue->device->device, eventHandle);
		}

		if (ed->setCallback) {
			ed->eventCallback = cacheDescription.eventCallback;
		}
	}

	void Delete(VulkanQueueImpl* queue) override {
		if (eventHandle != VK_NULL_HANDLE) {
			vkDestroyEvent(queue->device->device, eventHandle, queue->device->allocator);
		}
	}

	void Execute(VulkanQueueImpl* queue) override {
		if (cacheDescription.eventCallback) {
			queue->newEvents.Push(this);
		}

		vkCmdSetEvent(queue->currentCommandBuffer->buffer, eventHandle, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
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

struct PipelineKey {
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

static uint32_t ComputeBufferStride(const IRender::Resource::BufferDescription& desc) {
	uint32_t unit = 1;
	switch (desc.format) {
	case IRender::Resource::Description::UNSIGNED_BYTE:
		unit = 1;
		break;
	case IRender::Resource::Description::UNSIGNED_SHORT:
	case IRender::Resource::Description::HALF:
		unit = 2;
		break;
	case IRender::Resource::Description::FLOAT:
		unit = 3;
		break;
	}

	return unit * desc.component;
}


template <>
struct ResourceImplVulkan<IRender::Resource::RenderStateDescription> final : public ResourceImplVulkanDesc<IRender::Resource::RenderStateDescription> {
	virtual void Execute(VulkanQueueImpl* queue) override {
		queue->renderStateDescription = cacheDescription;
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_RENDERSTATE;
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
struct ResourceImplVulkan<IRender::Resource::RenderTargetDescription> final : public ResourceImplVulkanDesc<IRender::Resource::RenderTargetDescription> {
	ResourceImplVulkan() : signature(0), renderPass(VK_NULL_HANDLE), frameBuffer(VK_NULL_HANDLE) {}

	Resource::Type GetResourceType() override {
		return RESOURCE_RENDERTARGET;
	}

	const void* GetHandle() const override {
		return reinterpret_cast<const void*>(frameBuffer);
	}

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

	virtual void Delete(VulkanQueueImpl* queue) {
		VulkanDeviceImpl* device = queue->device;
		if (renderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device->device, renderPass, device->allocator);
			renderPass = VK_NULL_HANDLE;
		}

		if (frameBuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(device->device, frameBuffer, device->allocator);
			frameBuffer = VK_NULL_HANDLE;
		}
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* desc) override {
		BaseClass::Upload(queue, desc);
		signature = EncodeRenderTargetSignature(cacheDescription);

		// is backbuffer?
		if (cacheDescription.colorStorages.empty()) return;

		std::vector<VkAttachmentDescription> attachmentDescriptions(cacheDescription.colorStorages.size());
		std::vector<VkAttachmentReference> attachmentReferences(attachmentDescriptions.size());
		for (size_t i = 0; i < cacheDescription.colorStorages.size(); i++) {
			const IRender::Resource::RenderTargetDescription::Storage& storage = cacheDescription.colorStorages[i];
			ResourceImplVulkan<IRender::Resource::TextureDescription>* resource = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(storage.resource);

			VkAttachmentDescription& desc = attachmentDescriptions[i];
			desc.format = TranslateFormat(resource->cacheDescription.state.format, resource->cacheDescription.state.layout);
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = ConvertLoadOp(desc.loadOp);
			desc.storeOp = ConvertStoreOp(desc.storeOp);
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference& colorAttachment = attachmentReferences[i];
			colorAttachment.attachment = 0;
			colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		VkAttachmentReference depthStencilAttachment;
		depthStencilAttachment.attachment = 0;
		depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPass.colorAttachmentCount = verify_cast<uint32_t>(attachmentReferences.size());
		subPass.pColorAttachments = &attachmentReferences[0];
		subPass.pDepthStencilAttachment = &depthStencilAttachment;

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
		renderPassInfo.pAttachments = &attachmentDescriptions[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkRenderPass renderPass;
		VulkanDeviceImpl* device = queue->device;
		Verify("create render pass", vkCreateRenderPass(device->device, &renderPassInfo, device->allocator, &renderPass));

		std::vector<VkImageView> attachments(cacheDescription.colorStorages.size());
		for (size_t i = 0; i < attachments.size(); i++) {
			ResourceImplVulkan<IRender::Resource::TextureDescription>* texture = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(cacheDescription.colorStorages[i].resource);
			attachments[i] = texture->imageView;
		}

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = verify_cast<uint32_t>(cacheDescription.colorStorages.size());
		info.pAttachments = &attachments[0];
		info.layers = 1;

		if (cacheDescription.colorStorages.empty()) {
			info.width = device->resolution.x();
		} else {
			ResourceImplVulkan<TextureDescription>* texture = static_cast<ResourceImplVulkan<TextureDescription>*>(cacheDescription.colorStorages[0].resource);
			info.width = texture->cacheDescription.dimension.x();
		}
		if (cacheDescription.colorStorages.empty()) {
			info.height = device->resolution.y();
		} else {
			ResourceImplVulkan<TextureDescription>* texture = static_cast<ResourceImplVulkan<TextureDescription>*>(cacheDescription.colorStorages[0].resource);
			info.height = texture->cacheDescription.dimension.y();
		}

		Verify("create framebuffer", vkCreateFramebuffer(device->device, &info, device->allocator, &frameBuffer));
	}

	virtual void Execute(VulkanQueueImpl* queue) override {
		if (queue->renderTargetResource != nullptr) {
			vkCmdEndRenderPass(queue->currentCommandBuffer->buffer);
		}

		queue->renderTargetResource = this;
		VulkanDeviceImpl* device = queue->device;

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = renderPass;
		info.framebuffer = frameBuffer;
		info.renderArea.offset.x = cacheDescription.range.first.x();
		info.renderArea.offset.y = cacheDescription.range.second.y();

		UShort2Pair range = cacheDescription.range;

		if (range.second.x() == 0) {
			if (cacheDescription.colorStorages.empty()) {
				info.renderArea.extent.width = device->resolution.x();
			} else {
				ResourceImplVulkan<TextureDescription>* texture = static_cast<ResourceImplVulkan<TextureDescription>*>(cacheDescription.colorStorages[0].resource);
				info.renderArea.extent.width = texture->cacheDescription.dimension.x();
			}
		} else {
			info.renderArea.extent.width = range.second.x() - range.first.x();
		}

		if (range.second.y() == 0) {
			if (cacheDescription.colorStorages.empty()) {
				info.renderArea.extent.height = device->resolution.y();
			} else {
				ResourceImplVulkan<TextureDescription>* texture = static_cast<ResourceImplVulkan<TextureDescription>*>(cacheDescription.colorStorages[0].resource);
				info.renderArea.extent.height = texture->cacheDescription.dimension.y();
			}
		} else {
			info.renderArea.extent.height = range.second.y() - range.first.y();
		}

		info.clearValueCount = 1;

		std::vector<VkClearValue> clearValue(cacheDescription.colorStorages.size());
		for (size_t i = 0; i < cacheDescription.colorStorages.size(); i++) {
			VkClearValue& cv = clearValue[i];
			IRender::Resource::RenderTargetDescription::Storage& s = cacheDescription.colorStorages[i];
			static_assert(sizeof(cv.color.float32) == sizeof(s.clearColor), "Invalid color size");
			memcpy(&cv.color.float32, &s.clearColor, sizeof(s.clearColor));
		}

		info.pClearValues = &clearValue[0];

		VkViewport viewport;
		viewport.x = (float)info.renderArea.offset.x;
		viewport.y = (float)info.renderArea.offset.y;
		viewport.width = (float)info.renderArea.extent.width;
		viewport.height = (float)info.renderArea.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdBeginRenderPass(queue->currentCommandBuffer->buffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(queue->currentCommandBuffer->buffer, 0, 1, &viewport);
	}

	uint32_t signature;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;
};

template <>
struct ResourceImplVulkan<IRender::Resource::ShaderDescription> final : public ResourceImplVulkanBase {
	ResourceImplVulkan() : descriptorSetLayoutTexture(VK_NULL_HANDLE), descriptorSetLayoutUniformBuffer(VK_NULL_HANDLE), descriptorSetLayoutStorageBuffer(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {
		memset(shaderModules, 0, sizeof(shaderModules));
	}

	Resource::Type GetResourceType() override {
		return RESOURCE_SHADER;
	}

	void Clear(VulkanDeviceImpl* device) {
		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device->device, pipelineLayout, device->allocator);
			pipelineLayout = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutTexture != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device->device, descriptorSetLayoutTexture, device->allocator);
			descriptorSetLayoutTexture = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutUniformBuffer != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device->device, descriptorSetLayoutUniformBuffer, device->allocator);
			descriptorSetLayoutUniformBuffer = VK_NULL_HANDLE;
		}

		if (descriptorSetLayoutStorageBuffer != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device->device, descriptorSetLayoutStorageBuffer, device->allocator);
			descriptorSetLayoutStorageBuffer = VK_NULL_HANDLE;
		}

		for (size_t i = 0; i < sizeof(shaderModules) / sizeof(shaderModules[0]); i++) {
			VkShaderModule& m = shaderModules[i];
			if (m != VK_NULL_HANDLE) {
				vkDestroyShaderModule(device->device, m, device->allocator);
				m = VK_NULL_HANDLE;
			}
		}
	}

	void Download(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {}
	PipelineInstance& QueryInstance(VulkanQueueImpl* queue, ResourceImplVulkan<IRender::Resource::DrawCallDescription>* drawCall) {
		// Generate vertex format.
		const IRender::Resource::RenderStateDescription& renderState = queue->renderStateDescription;
		PipelineKey key;
		key.renderState = renderState;
		key.bufferSignature = drawCall->signature;
		assert(queue->renderTargetResource != nullptr);
		ResourceImplVulkan<IRender::Resource::RenderTargetDescription>* target = static_cast<ResourceImplVulkan<IRender::Resource::RenderTargetDescription>*>(queue->renderTargetResource);
		assert(target != nullptr);
		key.renderTargetSignature = target->signature;

		std::vector<std::key_value<PipelineKey, PipelineInstance> >::iterator it = std::binary_find(stateInstances.begin(), stateInstances.end(), key);
		if (it != stateInstances.end()) {
			return it->second;
		}

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

		std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources = drawCall->cacheDescription.bufferResources;
		uint32_t binding = 0;
		std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions;
		uint32_t location = 0;

		for (size_t k = 0; k < bufferResources.size(); k++) {
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = bufferResources[k];
			ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
			IRender::Resource::BufferDescription& desc = buffer->cacheDescription;

			uint32_t bindingIndex = drawCall->GetVertexBufferBindingIndex(verify_cast<uint32_t>(k));
			if (k == bindingIndex) {
				VkVertexInputBindingDescription bindingDesc = {};
				bindingDesc.binding = verify_cast<uint32_t>(inputBindingDescriptions.size());
				assert(bindingDesc.binding == k);
				bindingDesc.inputRate = desc.usage == IRender::Resource::BufferDescription::INSTANCED ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				bindingDesc.stride = desc.stride == 0 ? ComputeBufferStride(desc) : desc.stride;
				inputBindingDescriptions.emplace_back(bindingDesc);
			}

			// process component > 4
			for (uint32_t n = 0; n < desc.component; n += 4) {
				VkVertexInputAttributeDescription attributeDesc = {};
				attributeDesc.location = location++;
				attributeDesc.binding = bindingIndex;
				attributeDesc.format = TranslateFormat(n + 4 >= desc.component ? desc.component % 4 : 4, desc.format);
				inputAttributeDescriptions.emplace_back(std::move(attributeDesc));
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

		VkPipelineColorBlendStateCreateInfo blendInfo;
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.attachmentCount = verify_cast<uint32_t>(target->cacheDescription.colorStorages.size());
		assert(blendInfo.attachmentCount != 0);
		// TODO: different blend operations
		// create state instance
		VkPipelineColorBlendAttachmentState blendState = {};
		blendState.blendEnable = renderState.blend;
		if (blendState.blendEnable) {
			blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendState.colorBlendOp = VK_BLEND_OP_ADD;
			blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendState.alphaBlendOp = VK_BLEND_OP_ADD;
			blendState.colorWriteMask = renderState.colorWrite ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT : 0;
		}

		std::vector<VkPipelineColorBlendAttachmentState> blendStates(blendInfo.attachmentCount, blendState);
		blendInfo.pAttachments = &blendStates[0];

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
		info.pViewportState = &viewportInfo;
		info.pRasterizationState = &rasterInfo;
		info.pMultisampleState = &msInfo;
		info.pDepthStencilState = &depthInfo;
		info.pColorBlendState = &blendInfo;
		info.pDynamicState = &dynamicState;
		info.layout = pipelineLayout;
		info.renderPass = target->renderPass;

		PipelineInstance instance;
		VulkanDeviceImpl* device = queue->device;
		Verify("create graphics pipelines", vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &info, device->allocator, &instance.pipeline));

		return std::binary_insert(stateInstances, std::make_key_value(std::move(key), std::move(instance)))->second;
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
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

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		IRender::Resource::ShaderDescription& pass = *static_cast<IRender::Resource::ShaderDescription*>(d);
		VulkanDeviceImpl* device = queue->device;

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

		std::vector<VkDescriptorSetLayoutBinding> textureBindings;
		std::vector<VkDescriptorSetLayoutBinding> uniformBufferBindings;
		std::vector<VkDescriptorSetLayoutBinding> storageBufferBindings;
		std::vector<VkSampler> samplers;

		for (size_t k = 0; k < Resource::ShaderDescription::END; k++) {
			std::vector<IShader*>& pieces = shaders[k];
			if (pieces.empty()) continue;

			String body = "void main(void) {\n";
			String head = "";
			uint32_t inputIndex = 0, outputIndex = 0, textureIndex = 0;
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

			for (size_t n = 0; n < pieces.size(); n++) {
				IShader* shader = pieces[n];
				// Generate declaration
				GLSLShaderGenerator declaration((Resource::ShaderDescription::Stage)k, inputIndex, outputIndex, textureIndex);
				(*shader)(declaration);
				declaration.Complete();

				body += declaration.initialization + shader->GetShaderText() + declaration.finalization + "\n";
				head += declaration.declaration;

				for (size_t k = 0; k < declaration.bufferBindings.size(); k++) {
					std::pair<const IShader::BindBuffer*, String>& item = declaration.bufferBindings[k];
					const IShader::BindBuffer* bindBuffer = item.first;
					bufferDescriptions.emplace_back(bindBuffer->description);
					if (bindBuffer->description.usage == IRender::Resource::BufferDescription::UNIFORM || bindBuffer->description.usage == IRender::Resource::BufferDescription::STORAGE) {
						VkDescriptorSetLayoutBinding binding = {};
						binding.binding = verify_cast<uint32_t>(uniformBufferBindings.size());
						binding.descriptorCount = 1;
						binding.descriptorType = bindBuffer->description.usage == IRender::Resource::BufferDescription::UNIFORM ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						binding.stageFlags = stageFlags;
						binding.pImmutableSamplers = nullptr;

						if (bindBuffer->description.usage == IRender::Resource::BufferDescription::UNIFORM) {
							uniformBufferBindings.emplace_back(std::move(binding));
						} else {
							storageBufferBindings.emplace_back(std::move(binding));
						}
					}
				}

				size_t startSamplerIndex = samplers.size();
				for (size_t m = 0; m < declaration.textureBindings.size(); m++) {
					std::pair<const IShader::BindTexture*, String>& item = declaration.textureBindings[m];
					const IRender::Resource::TextureDescription::State& state = item.first->description.state;

					VkSamplerCreateInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
					info.minFilter = info.magFilter = state.sample == IRender::Resource::TextureDescription::POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
					info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; // not tri-filter
					info.addressModeU = ConvertAddressMode(info.addressModeU);
					info.addressModeV = ConvertAddressMode(info.addressModeV);
					info.addressModeW = ConvertAddressMode(info.addressModeW);
					info.minLod = -1000;
					info.maxLod = 1000;
					info.maxAnisotropy = 4.0f;
					info.anisotropyEnable = state.sample == IRender::Resource::TextureDescription::ANSOTRIPIC;

					VkSampler sampler;
					Verify("create sampler", vkCreateSampler(device->device, &info, device->allocator, &sampler));
					samplers.emplace_back(sampler);
				}

				if (!declaration.textureBindings.empty()) {
					VkDescriptorSetLayoutBinding binding;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.descriptorCount = (uint32_t)declaration.textureBindings.size();
					binding.stageFlags = stageFlags;
					binding.binding = (uint32_t)textureBindings.size();

					binding.pImmutableSamplers = reinterpret_cast<VkSampler*>(startSamplerIndex);
					textureBindings.emplace_back(binding);
				}
			}

			body += "\n}\n"; // make a call to our function

			String fullShader = GLSLShaderGenerator::GetFrameCode() + common + head + body;
			// Vulkan only support SPIRV shader
			fullShader = SPIRVCompiler::Compile((IRender::Resource::ShaderDescription::Stage)k, fullShader);

			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = fullShader.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(fullShader.c_str());

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
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = descriptorSetLayoutTexture ? 1 : 0;
		layoutInfo.pSetLayouts = &descriptorSetLayoutTexture;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;

		Verify("create pipeline layout", vkCreatePipelineLayout(device->device, &layoutInfo, device->allocator, &pipelineLayout));

		// Format descriptor layout sets

		// Fix sampler pointers
		for (size_t j = 0; j < textureBindings.size(); j++) {
			VkDescriptorSetLayoutBinding& binding = textureBindings[j];
			binding.pImmutableSamplers = &samplers[*(uint32_t*)&textureBindings[j].pImmutableSamplers];
		}

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
	}

	virtual void Execute(VulkanQueueImpl* queue) override {
		assert(false); // not possible
	}

	VkDescriptorSetLayout descriptorSetLayoutTexture;
	VkDescriptorSetLayout descriptorSetLayoutUniformBuffer;
	VkDescriptorSetLayout descriptorSetLayoutStorageBuffer;
	VkPipelineLayout pipelineLayout;
	VkShaderModule shaderModules[IRender::Resource::ShaderDescription::Stage::END];

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
	std::vector<IRender::Resource::BufferDescription> bufferDescriptions;
	std::vector<std::key_value<PipelineKey, PipelineInstance> > stateInstances;
};

// Resource
IRender::Resource* ZRenderVulkan::CreateResource(Device* device, Resource::Type resourceType) {
	switch (resourceType)
	{
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
	case Resource::RESOURCE_EVENT:
		return new ResourceImplVulkan<Resource::EventDescription>();
	}

	assert(false);
	return nullptr;
}

void ZRenderVulkan::UploadResource(Queue* queue, Resource* resource, Resource::Description* description) {
	ResourceImplVulkanBase* res = static_cast<ResourceImplVulkanBase*>(resource);
	res->Upload(static_cast<VulkanQueueImpl*>(queue), description);
}

void ZRenderVulkan::SetupBarrier(Queue* queue, Barrier* barrier) {
	// memory barrier?
	VulkanQueueImpl* q = static_cast<VulkanQueueImpl*>(queue);
	ResourceImplVulkanBase* resource = static_cast<ResourceImplVulkanBase*>(barrier->resource);

	if (resource == nullptr) {
		VkMemoryBarrier memoryBarrier;
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.pNext = nullptr;
		memoryBarrier.srcAccessMask = (VkAccessFlags)barrier->srcAccessMask; // totally mapped
		memoryBarrier.dstAccessMask = (VkAccessFlags)barrier->dstAccessMask; // totally mapped

		vkCmdPipelineBarrier(q->currentCommandBuffer->buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	} else {
		Resource::Type resourceType = resource->GetResourceType();
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
			memoryBarrier.size = barrier->size;

			vkCmdPipelineBarrier(q->currentCommandBuffer->buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 0, nullptr, 1, &memoryBarrier, 0, nullptr);
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

			vkCmdPipelineBarrier(q->currentCommandBuffer->buffer, (VkPipelineStageFlags)barrier->srcStageMask, (VkPipelineStageFlags)barrier->dstStageMask, (VkDependencyFlags)barrier->dependencyMask, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
		} else {
			assert(false);
		}
	}
}

void ZRenderVulkan::RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) {
	ResourceImplVulkanBase* impl = static_cast<ResourceImplVulkanBase*>(resource);
	impl->Download(static_cast<VulkanQueueImpl*>(queue), description);
}

void ZRenderVulkan::CompleteDownloadResource(Queue* queue, Resource* resource) {

}

void ZRenderVulkan::ExecuteResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::SetResourceNotation(Resource* lhs, const String& note) {}

void ZRenderVulkan::DeleteResource(Queue* queue, Resource* resource) {
	VulkanQueueImpl* impl = static_cast<VulkanQueueImpl*>(queue);
	impl->DoLock();
	impl->deletedResources.Push(static_cast<ResourceImplVulkanBase*>(resource));
	impl->UnLock();
}

static inline VkBuffer GetBuffer(IRender::Resource::DrawCallDescription::BufferRange& range) {
	ResourceImplVulkan<IRender::Resource::BufferDescription>* bufferResource = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(range.buffer);
	return bufferResource->buffer;
}

void ResourceImplVulkan<IRender::Resource::DrawCallDescription>::Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) {
	// Compute signature
	IRender::Resource::DrawCallDescription* description = static_cast<IRender::Resource::DrawCallDescription*>(d);
	signature.Resize(description->bufferResources.size() * 3);
	uint8_t* data = signature.GetData();
	ResourceImplVulkan<IRender::Resource::ShaderDescription>* shader = static_cast<ResourceImplVulkan<IRender::Resource::ShaderDescription>*>(description->shaderResource);

	VulkanDeviceImpl* device = queue->device;
	if (descriptorSetTexture.second == VK_NULL_HANDLE && shader->descriptorSetLayoutTexture != VK_NULL_HANDLE) {
		descriptorSetTexture = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER].AllocateDescriptorSet(shader->descriptorSetLayoutTexture);
	}

	if (descriptorSetUniformBuffer.second == VK_NULL_HANDLE && shader->descriptorSetLayoutUniformBuffer != VK_NULL_HANDLE) {
		descriptorSetUniformBuffer = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER].AllocateDescriptorSet(shader->descriptorSetLayoutUniformBuffer);
	}

	if (descriptorSetStorageBuffer.second == VK_NULL_HANDLE && shader->descriptorSetLayoutStorageBuffer != VK_NULL_HANDLE) {
		descriptorSetStorageBuffer = device->descriptorAllocators[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER].AllocateDescriptorSet(shader->descriptorSetLayoutStorageBuffer);
	}

	// upload bindings
	std::vector<VkDescriptorBufferInfo> uniformBufferInfos;
	std::vector<VkDescriptorBufferInfo> storageBufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;

	for (size_t i = 0; i < description->bufferResources.size(); i++) {
		const IRender::Resource::DrawCallDescription::BufferRange& bufferRange = description->bufferResources[i];
		assert(bufferRange.buffer != nullptr);

		ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
		IRender::Resource::BufferDescription& desc = buffer->cacheDescription;
		if (desc.usage == IRender::Resource::BufferDescription::VERTEX || desc.usage == IRender::Resource::BufferDescription::INSTANCED) {
			assert(desc.component < 32);
			assert(bufferRange.offset <= 255);

			uint8_t sameIndex = verify_cast<uint8_t>(i);
			for (size_t n = 0; n < i; n++) {
				if (description->bufferResources[n].buffer == buffer) {
					sameIndex = verify_cast<uint8_t>(n);
					break;
				}
			}

			*data++ = sameIndex;
			*data++ = desc.format << 5 | desc.component;
			*data++ = verify_cast<uint8_t>(bufferRange.offset);
		} else if (desc.usage == IRender::Resource::BufferDescription::UNIFORM || desc.usage == IRender::Resource::BufferDescription::STORAGE) {
			VkDescriptorBufferInfo info = {};
			info.buffer = buffer->buffer;
			info.offset = bufferRange.offset;
			info.range = bufferRange.length;

			if (desc.usage == IRender::Resource::BufferDescription::UNIFORM) {
				uniformBufferInfos.emplace_back(std::move(info));
			} else {
				storageBufferInfos.emplace_back(std::move(info));
			}
		} else {
			assert(false); // not supported by now
		}
	}

	for (size_t i = 0; i < description->textureResources.size(); i++) {
		ResourceImplVulkan<IRender::Resource::TextureDescription>* texture = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(description->textureResources[i]);

		VkDescriptorImageInfo info = {};
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		info.imageView = texture->imageView;
		info.sampler = VK_NULL_HANDLE; // use immutable samplers

		imageInfos.emplace_back(std::move(info));
	}

	if (uniformBufferInfos.empty()) {
		if (!imageInfos.empty()) {
			VkWriteDescriptorSet desc = {};
			desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			desc.dstSet = descriptorSetTexture.second;
			desc.descriptorCount = verify_cast<uint32_t>(imageInfos.size());
			desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			desc.pImageInfo = &imageInfos[0];

			vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
		}

		if (!uniformBufferInfos.empty()) {
			VkWriteDescriptorSet desc = {};
			desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			desc.dstSet = descriptorSetUniformBuffer.second;
			desc.descriptorCount = verify_cast<uint32_t>(uniformBufferInfos.size());
			desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			desc.pBufferInfo = &uniformBufferInfos[0];
			vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
		}

		if (!storageBufferInfos.empty()) {
			VkWriteDescriptorSet desc = {};
			desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			desc.dstSet = descriptorSetStorageBuffer.second;
			desc.descriptorCount = verify_cast<uint32_t>(storageBufferInfos.size());
			desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			desc.pBufferInfo = &storageBufferInfos[0];
			vkUpdateDescriptorSets(device->device, 1, &desc, 0, nullptr);
		}
	}

	BaseClass::Upload(queue, description);
}


void ResourceImplVulkan<IRender::Resource::DrawCallDescription>::Execute(VulkanQueueImpl* queue) {
	ResourceImplVulkan<IRender::Resource::ShaderDescription>* shaderResource = static_cast<ResourceImplVulkan<IRender::Resource::ShaderDescription>*>(cacheDescription.shaderResource);

	PipelineInstance& pipelineInstance = shaderResource->QueryInstance(queue, this);
	VkCommandBuffer commandBuffer = queue->currentCommandBuffer->buffer;
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineInstance.pipeline);
	VkDescriptorSet descriptors[3];
	uint32_t i = 0;
	if (descriptorSetTexture.second != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetTexture.second;
	}

	if (descriptorSetUniformBuffer.second != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetUniformBuffer.second;
	}

	if (descriptorSetStorageBuffer.second != VK_NULL_HANDLE) {
		descriptors[i++] = descriptorSetStorageBuffer.second;
	}

	if (i != 0) {
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderResource->pipelineLayout, 0, i, descriptors, 0, nullptr);
	}

	vkCmdBindIndexBuffer(commandBuffer, static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(cacheDescription.indexBufferResource.buffer)->buffer, cacheDescription.indexBufferResource.offset, VK_INDEX_TYPE_UINT32);

	std::vector<VkBuffer> vertexBuffers;
	for (size_t k = 0; k < cacheDescription.bufferResources.size(); k++) {
		ResourceImplVulkan<IRender::Resource::BufferDescription>* p = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(cacheDescription.bufferResources[k].buffer);
		if (p->cacheDescription.usage == IRender::Resource::BufferDescription::VERTEX) {
			vertexBuffers.emplace_back(p->buffer);
		}
	}

	vkCmdBindVertexBuffers(commandBuffer, 0, verify_cast<uint32_t>(vertexBuffers.size()), &vertexBuffers[0], nullptr);
	vkCmdDrawIndexed(commandBuffer, cacheDescription.indexBufferResource.length / sizeof(UInt3), cacheDescription.instanceCounts.x(), 0, 0, 0);
}

void VulkanQueueImpl::DispatchEvents() {
	while (!pendingEvents.Empty()) {
		ResourceImplVulkan<IRender::Resource::EventDescription>* ev = static_cast<ResourceImplVulkan<IRender::Resource::EventDescription>*>(pendingEvents.Top());
		if (ev->eventHandle != VK_NULL_HANDLE) {
			if (vkGetEventStatus(device->device, ev->eventHandle) == VK_EVENT_SET) {
				if (ev->cacheDescription.eventCallback) {
					ev->cacheDescription.eventCallback(device->render, this);
				}
			} else {
				pendingEvents.Push(ev); // reenter
			}
		}

		pendingEvents.Pop();
	}

	while (!newEvents.Empty()) {
		ResourceImplVulkan<IRender::Resource::EventDescription>* ev = static_cast<ResourceImplVulkan<IRender::Resource::EventDescription>*>(newEvents.Top());
		if (ev->eventHandle != VK_NULL_HANDLE) {
			if (vkGetEventStatus(device->device, ev->eventHandle) == VK_EVENT_SET) {
				if (ev->cacheDescription.eventCallback) {
					ev->cacheDescription.eventCallback(device->render, this);
				}
			} else {
				pendingEvents.Push(ev);
			}
		}

		newEvents.Pop();
	}
}

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
	fprintf(stderr, "Vulkan debug: %d - %s\n", objectType, pMessage);
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

#ifdef _DEBUG 
	// Enabling multiple validation layers grouped as LunarG standard validation
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;

	// Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
	std::vector<const char*> extensions_ext(extensionsCount + 1);
	memcpy(&extensions_ext[0], extensions, extensionsCount * sizeof(const char*));
	extensions_ext[extensionsCount] = "VK_EXT_debug_report";
	createInfo.enabledExtensionCount = extensionsCount + 1;
	createInfo.ppEnabledExtensionNames = &extensions_ext[0];

	// Create Vulkan Instance
	VkAllocationCallbacks* allocator = nullptr; // TODO: customize allocators
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

const void* ZRenderVulkan::GetResourceDeviceHandle(IRender::Resource* resource) {
	assert(resource != nullptr);
	ResourceImplVulkanBase* p = static_cast<ResourceImplVulkanBase*>(resource);

	return p->GetHandle();
}

ZRenderVulkan::~ZRenderVulkan() {
#ifdef _DEBUG
	VkAllocationCallbacks* allocator = nullptr; // TODO: customize allocators
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(instance, (VkDebugReportCallbackEXT)debugCallback, allocator);
#endif

	vkDestroyInstance(instance, nullptr);
}
