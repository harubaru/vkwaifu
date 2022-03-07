#define VOLK_IMPLEMENTATION
#include "volk.h"
#include "vulkanctx.h"

#include "vert.h"
#include "frag.h"
#include <cstring>

#ifndef _DEBUG
//#define _DEBUG
#endif

extern VkResult volkInitialize(void);

#ifdef _DEBUG
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *data, void *userdata)
	{
		std::cerr << data->pMessage << std::endl;

		return VK_FALSE;
	}
#endif


// Helper functions

static uint32_t getQueueFamily(uint32_t starting_queue, int flags, std::vector<VkQueueFamilyProperties> &props)
{
	for (uint32_t i = starting_queue; i < props.size(); i++) {
		if (props[i].queueFlags & flags) {
			return i;
		}
	}

	return VK_QUEUE_FAMILY_IGNORED;
}

VkShaderModule createShaderModule(VkDevice device, const uint32_t *spvCode, size_t spvSize)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spvSize;
	createInfo.pCode = spvCode;

	VkShaderModule shaderModule;
	VK_ASSERT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create Shader Module")

	return shaderModule;
}

uint32_t memoryType(VkPhysicalDevice physicalDevice, uint32_t type, VkMemoryPropertyFlags props)
{
	VkPhysicalDeviceMemoryProperties physDevMemProps;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physDevMemProps);

	for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i) {
		if ((type & (1 << i)) && ((physDevMemProps.memoryTypes[i].propertyFlags & props) == props))
			return i;
	}

	return VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM;
}

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize dataSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkBuffer *buffer, VkDeviceMemory *bufferMemory, uint32_t *queueFamilyIndices, uint32_t queueFamilyCount)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = dataSize;
	bufferCreateInfo.usage = usageFlags;
	
	if ((memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) || (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	} else {
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferCreateInfo.queueFamilyIndexCount = queueFamilyCount;
		bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	VK_ASSERT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer), "Failed to create Device Buffer")

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = memoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryFlags);

	VK_ASSERT(vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory), "Failed to allocate memory for device buffer");
	VK_ASSERT(vkBindBufferMemory(device, *buffer, *bufferMemory, 0), "Failed to bind memory for device buffer")
}

void copyBufferCmd(VkDeviceSize dataSize, VkBuffer src, VkBuffer dst, VkCommandBuffer commandBuffer)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = dataSize;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);
}

void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkImage *image, VkDeviceMemory *imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usageFlags;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_ASSERT(vkCreateImage(device, &imageInfo, nullptr, image), "Failed to create Texture2D!")

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, *image, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = memoryType(physicalDevice, memoryRequirements.memoryTypeBits, memoryFlags);

	VK_ASSERT(vkAllocateMemory(device, &allocInfo, nullptr, imageMemory), "Failed to allocate Texture2D memory!");

	vkBindImageMemory(device, *image, *imageMemory, 0);
}

void transitionImageLayoutCmd(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier imageBarrier = {};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = oldLayout;
	imageBarrier.newLayout = newLayout;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = image;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) { // Transfer from host to device
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) { // Write to shader resource view
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		VK_FATAL(1, "Unsupported layout transition!")
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

void copyBufferToImageCmd(uint32_t width, uint32_t height, VkBuffer buffer, VkImage image, VkCommandBuffer commandBuffer)
{
	VkBufferImageCopy regionCopy = {};
	regionCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	regionCopy.imageSubresource.layerCount = 1;
	regionCopy.imageOffset = {0, 0, 0};
	regionCopy.imageExtent.width = width;
	regionCopy.imageExtent.height = height;
	regionCopy.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionCopy);
}

bool VulkanCTX::Setup(int width, int height)
{
	ResetCache();

	// Create Instance
	glfwInit();
	if (volkInitialize() != VK_SUCCESS) 
		return false;

	uint32_t extensionCount = 0;
	const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	std::vector<const char *> requiredExtensions(extensions, extensions + extensionCount); 

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "vkwaifu";
	appInfo.applicationVersion = 0;
	appInfo.pEngineName = "vkwaifu";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

	VkInstanceCreateInfo instInfo = {};
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instInfo.pNext = nullptr;
	instInfo.flags = 0;
	instInfo.pApplicationInfo = &appInfo;

#ifdef _DEBUG
	requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	std::vector<const char *>layernames = {
		"VK_LAYER_KHRONOS_validation"
	};

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debugCallback;

	instInfo.enabledLayerCount = layernames.size();
	instInfo.ppEnabledLayerNames = layernames.data();
	instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugMessengerInfo;
#endif

	instInfo.enabledExtensionCount = requiredExtensions.size();
	instInfo.ppEnabledExtensionNames = requiredExtensions.data();

	VK_ASSERT(vkCreateInstance(&instInfo, nullptr, &instance), "Failed to create instance!");
	volkLoadInstance(instance);

#ifdef _DEBUG
	VK_ASSERT(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger), "Failed to create debugger messenger callback")
#endif

	// Fetch a device!

	uint32_t physicalDeviceCount;
	std::vector<VkPhysicalDevice> physicalDevices;
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr), "Failed to query amount of physical devices")
	physicalDevices.resize(physicalDeviceCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()), "Failed to query physical devices")

	for (auto &physDev : physicalDevices) {
		VkPhysicalDeviceProperties physDevProps;
		vkGetPhysicalDeviceProperties(physDev, &physDevProps);

		if (physDevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			physicalDev = physDev;
			break;
		}
	}

	if (!physicalDev)
		return false;

	uint32_t queueFamilyCount = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyProps;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &queueFamilyCount, nullptr);
	queueFamilyProps.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &queueFamilyCount, queueFamilyProps.data());

	// We're only going to use 2 queues, graphics and transfer
	graphicsQueues.resize(1);
	transferQueues.resize(1);

	float queuePriorities = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfos[2];
	
	queueCreateInfos[0] = {};
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = nullptr;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].queueFamilyIndex = getQueueFamily(0, VK_QUEUE_GRAPHICS_BIT, queueFamilyProps);
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &queuePriorities;

	queueCreateInfos[1] = {};
	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].pNext = nullptr;
	queueCreateInfos[1].flags = 0;
	queueCreateInfos[1].queueFamilyIndex = getQueueFamily(1, ~VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_TRANSFER_BIT, queueFamilyProps);
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = &queuePriorities;
	
	std::vector<const char *> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkPhysicalDeviceFeatures physDevEnabledFeatures = {};
	physDevEnabledFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo devCreateInfo = {};
	devCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devCreateInfo.pNext = nullptr;
	devCreateInfo.flags = 0;
	devCreateInfo.queueCreateInfoCount = 2;
	devCreateInfo.pQueueCreateInfos = queueCreateInfos;
	devCreateInfo.enabledExtensionCount = deviceExtensions.size();
	devCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	devCreateInfo.pEnabledFeatures = &physDevEnabledFeatures;

	VK_ASSERT(vkCreateDevice((physicalDev), &devCreateInfo, nullptr, &device), "Failed to create device")

	for (uint32_t i = 0; i < queueCreateInfos[0].queueCount; i++) {
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(device, queueCreateInfos[0].queueFamilyIndex, i, &graphicsQueue);
		graphicsQueues[i] = graphicsQueue;
	}
	graphicsQueueFamily = queueCreateInfos[0].queueFamilyIndex;

	for (uint32_t i = 0; i < queueCreateInfos[1].queueCount; i++) {
		VkQueue transferQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(device, queueCreateInfos[1].queueFamilyIndex, i, &transferQueue);
		transferQueues[i] = transferQueue;
	}
	transferQueueFamily = queueCreateInfos[1].queueFamilyIndex;

	VkCommandPoolCreateInfo graphicsPoolCreateInfo = {};
	graphicsPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolCreateInfo.queueFamilyIndex = queueCreateInfos[0].queueFamilyIndex;
	graphicsPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_ASSERT(vkCreateCommandPool(device, &graphicsPoolCreateInfo, nullptr, &graphicsPool), "Failed to create Graphics Command Pool")

	VkCommandPoolCreateInfo transferPoolCreateInfo = {};
	transferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolCreateInfo.queueFamilyIndex = queueCreateInfos[1].queueFamilyIndex;
	transferPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VK_ASSERT(vkCreateCommandPool(device, &transferPoolCreateInfo, nullptr, &transferPool), "Failed to create Transfer Command Pool")

	// Now, lets setup the swapchain!

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, "vkwaifu: waifuing edition!", nullptr, nullptr);
	VK_ASSERT(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create window surface");

	VkBool32 supported;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, queueCreateInfos[0].queueFamilyIndex, surface, &supported), "surface got lost on its way to vkwaifu")
	VK_FATAL(supported != VK_TRUE, "Device does not support presentation")

	// Create Uniform Buffer Object
	VkDeviceSize uboSize = sizeof(VulkanUBO);
	createBuffer(device, physicalDev, uboSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, &uniformBufferMemory, nullptr, 0);

	// Create Descriptor Set Layout first
	VkDescriptorSetLayoutBinding layoutBindings[2] = {{}, {}};

	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = layoutBindings;

	VK_ASSERT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorLayout), "Failed to create Descriptor Layout!")

	// Create Pipeline Layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorLayout;

	VK_ASSERT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create Pipeline Layout")

	// Setup descriptor pool and descriptor set
	VkDescriptorPoolSize poolSizes[2]; // 1 descriptor per binding. 0 = ubo, 1 = sampler

	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 1; // 1 set only

	VK_ASSERT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "Failed to create Descriptor Pool!")

	// create descriptor sets
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &descriptorLayout;

	VK_ASSERT(vkAllocateDescriptorSets(device, &setAllocInfo, &descriptorSet), "Failed to allocate Descriptor Set!")

	return true;
}

bool VulkanCTX::Resize() // resizes swapchain
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	while (!width || !height) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	// Get format and present mode beforehand
	uint32_t formatCount = 0;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &formatCount, nullptr);
	surfaceFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, surface, &formatCount, surfaceFormats.data());

	surfaceFormat.format = VK_FORMAT_UNDEFINED;

	for (auto &i : surfaceFormats) {
		if ((i.format  == SURFACE_FORMAT) && (i.colorSpace == SURFACE_COLORSPACE)) {
			surfaceFormat = i;
			break;
		}
	}

	VK_FATAL(surfaceFormat.format == VK_FORMAT_UNDEFINED, "cannot find supported format")

	uint32_t presentModeCount = 0;
	std::vector<VkPresentModeKHR> presentModes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &presentModeCount, nullptr);
	presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, surface, &presentModeCount, presentModes.data());

	presentMode = (VkPresentModeKHR)0xFF; // garbage value for checking

	for (auto &i : presentModes) {
		if (i == PRESENT_MODE) {
			presentMode = i;
			break;
		}
	}

	VK_FATAL(presentMode == 0xFF, "cannot find supported surface present mode")

	// Create swapchain
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDev, surface, &surfaceCapabilities);

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = swapchain;

	VK_ASSERT(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain), "failed to create swapchain")

	// Get swapchain images and create image views.
	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
	swapchainImages.resize(swapchainImageCount);
	swapchainImageViews.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

	// Destroy old objects such as materials
	if (swapchainCreateInfo.oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
		vkFreeCommandBuffers(device, graphicsPool, static_cast<uint32_t>(presentCommandBuffer.size()), presentCommandBuffer.data());

		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);

		// Don't free textures or uniforms

		for (uint32_t i = 0; i < swapchainImageCount; i++) {
			vkDestroyFramebuffer(device, framebuffers[i], nullptr);
			vkDestroyImageView(device, swapchainImageViews[i], nullptr);
			vkDestroySemaphore(device, acquireSemaphores[i], nullptr);
			vkDestroySemaphore(device, presentSemaphores[i], nullptr);
			vkDestroyFence(device, fences[i], nullptr);
			inFlightFences[i] = VK_NULL_HANDLE;
		}
	}


	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VkImageViewCreateInfo swapchainViewCreateInfo = {};
		swapchainViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		swapchainViewCreateInfo.image = swapchainImages[i];
		swapchainViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		swapchainViewCreateInfo.format = surfaceFormat.format;
		swapchainViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
		swapchainViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchainViewCreateInfo.subresourceRange.baseMipLevel = 0;
		swapchainViewCreateInfo.subresourceRange.levelCount = 1;
		swapchainViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		swapchainViewCreateInfo.subresourceRange.layerCount = 1;

		VK_ASSERT(vkCreateImageView(device, &swapchainViewCreateInfo, nullptr, &swapchainImageViews[i]), "Failed to create Swapchain Image View!")
	}

	// Create one set of semaphores and fences per image.

	fences.resize(swapchainImageCount);
	inFlightFences.resize(swapchainImageCount);
	acquireSemaphores.resize(swapchainImageCount);
	presentSemaphores.resize(swapchainImageCount);
	presentCommandBuffer.resize(swapchainImageCount);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VK_ASSERT(
			vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &acquireSemaphores[i]) ||
			vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &presentSemaphores[i]) ||
			vkCreateFence(device, &fenceCreateInfo, nullptr, &fences[i]),
			"Failed to create synchronization objects!"
		);
	}

	VkCommandBufferAllocateInfo commandbufferAllocateInfo = {};
	commandbufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandbufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandbufferAllocateInfo.commandPool = graphicsPool;
	commandbufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(presentCommandBuffer.size());

	VK_ASSERT(vkAllocateCommandBuffers(device, &commandbufferAllocateInfo, presentCommandBuffer.data()), "Failed to allocate Command Buffers for Presentation")

	// Create renderpass!

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VK_ASSERT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass), "Failed to create Render Pass!")


	// Create framebuffers

	framebuffers.resize(swapchainImageCount);

	for (uint32_t i = 0; i < swapchainImageCount; i++) {
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &swapchainImageViews[i];
		framebufferInfo.width = surfaceCapabilities.currentExtent.width;
		framebufferInfo.height = surfaceCapabilities.currentExtent.height;
		framebufferInfo.layers = 1;

		VK_ASSERT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]), "Failed to create framebuffers for swapchain")
	} 

	SetupGraphics(surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height);
	swapExtent = surfaceCapabilities.currentExtent;

	return true;
}

void VulkanCTX::Release() // destroys vulkanctx
{
	vkDeviceWaitIdle(device);

	vkFreeCommandBuffers(device, graphicsPool, static_cast<uint32_t>(presentCommandBuffer.size()), presentCommandBuffer.data());

	for (uint32_t i = 0; i < swapchainImageViews.size(); i++) {
		vkDestroyFramebuffer(device, framebuffers[i], nullptr);
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
		vkDestroySemaphore(device, acquireSemaphores[i], nullptr);
		vkDestroySemaphore(device, presentSemaphores[i], nullptr);
		vkDestroyFence(device, fences[i], nullptr);
	}

	vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);

	ReleaseTexture();

	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyCommandPool(device, transferPool, nullptr);
	vkDestroyCommandPool(device, graphicsPool, nullptr);
	vkDestroyDevice(device, nullptr);
#ifdef _DEBUG
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
	vkDestroyInstance(instance, nullptr);

	ResetCache();
}

void VulkanCTX::ResetCache() // clears internal cache
{
	instance = VK_NULL_HANDLE;
	device = VK_NULL_HANDLE;
	debugMessenger = VK_NULL_HANDLE;
	physicalDev = VK_NULL_HANDLE;
	
	graphicsPool = VK_NULL_HANDLE;
	transferPool = VK_NULL_HANDLE;

	window = nullptr;
	surface = VK_NULL_HANDLE;
	swapchain = VK_NULL_HANDLE;

	currentImage = 0;
}

void VulkanCTX::Present() // presents to screen
{
	VkPipelineStageFlags waitDst = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &acquireSemaphores[currentImage];
	submitInfo.pWaitDstStageMask = &waitDst;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &presentCommandBuffer[currentImage];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &presentSemaphores[currentImage]; // when done, signal present semaphore.

	VK_ASSERT(vkQueueSubmit(graphicsQueues[0], 1, &submitInfo, fences[currentImage]), "Failed to submit to presentation command buffer")

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &presentSemaphores[currentImage];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = (const uint32_t *)&imageIndex;

	VkResult res = vkQueuePresentKHR(graphicsQueues[0], &presentInfo);
	if ((res == VK_ERROR_OUT_OF_DATE_KHR) || (res == VK_SUBOPTIMAL_KHR)) {
		vkDeviceWaitIdle(device);
		this->Resize();
		return;
	} else if (res != VK_SUCCESS) {
		std::cerr << "Failed to present queue!" << std::endl;
		std::abort();
	}	

	currentImage = (currentImage + 1) % swapchainImages.size();
}

void VulkanCTX::Update() // updates swapchain
{
	vkWaitForFences(device, 1, &fences[currentImage], VK_TRUE, UINT64_MAX);
	VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSemaphores[currentImage], VK_NULL_HANDLE, &imageIndex);

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {		
		vkDeviceWaitIdle(device);
		this->Resize();
		return;
	} else if (res != VK_SUCCESS) {
		std::cerr << "Failed to acquire next swapchain image!" << std::endl;
		std::abort();
	}

	vkResetFences(device, 1, &fences[currentImage]);
}

void VulkanCTX::ClearCurrentImage()
{
	VkClearValue clearColor = {0.0f, 0.5f, 0.4f, 1.0f};

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffers[currentImage];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(this->getCurrentCommandBuffer(), &beginInfo);
	vkCmdBeginRenderPass(this->getCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(this->getCurrentCommandBuffer());
	vkEndCommandBuffer(this->getCurrentCommandBuffer());
}

void VulkanCTX::SetupGraphics(uint32_t width, uint32_t height)
{
	// Feed shaders into pipeline

	VkShaderModule vsShader = createShaderModule(device, vsSpv, sizeof(vsSpv));
	VkShaderModule fsShader = createShaderModule(device, fsSpv, sizeof(fsSpv));

	VkPipelineShaderStageCreateInfo shaderStages[2];

	shaderStages[0] = {};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vsShader;
	shaderStages[0].pName = "main";

	shaderStages[1] = {};
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fsShader;
	shaderStages[1].pName = "main";

	// Feed input layout

	VkPipelineVertexInputStateCreateInfo vertexInputLayout = {};
	vertexInputLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputLayout = {};
	inputLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputLayout.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Feed in viewport info
	
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width =  static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent.width =  width;
	scissor.extent.height = height;

	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	// Feed in rasterizer state

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//	rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
//	rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Feed in blend state

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	
	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputLayout;
	pipelineInfo.pInputAssemblyState = &inputLayout;
	pipelineInfo.pViewportState = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizerState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	
	VK_ASSERT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create Graphics Pipeline")

	vkDestroyShaderModule(device, vsShader, nullptr);
	vkDestroyShaderModule(device, fsShader, nullptr);
}

void VulkanCTX::DrawGraphics()
{
	VkClearValue clearColor = {0.0f, 0.5f, 0.4f, 1.0f};

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = framebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapExtent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(this->getCurrentCommandBuffer(), &beginInfo);
	vkCmdBeginRenderPass(this->getCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(this->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(this->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdDraw(this->getCurrentCommandBuffer(), 6, 1, 0, 0);

	vkCmdEndRenderPass(this->getCurrentCommandBuffer());
	vkEndCommandBuffer(this->getCurrentCommandBuffer());
}

void VulkanCTX::SetupTexture(uint8_t *data, uint32_t width, uint32_t height)
{
	if (!data)
		return;
	
	VkDeviceSize dataSize = width * height * 4;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	uint32_t queueFamilies[2] = {
		graphicsQueueFamily,
		transferQueueFamily
	};

	createBuffer(device, physicalDev, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory, queueFamilies, 2);

	void *mappedData;
	vkMapMemory(device, stagingBufferMemory, 0, dataSize, 0, &mappedData);
	memcpy(mappedData, data, static_cast<size_t>(dataSize));
	vkUnmapMemory(device, stagingBufferMemory);

	// create image
	createImage(device, physicalDev, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureMemory);

	// Prepare command buffer
	VkCommandBufferAllocateInfo commandBufferInfo = {};
	commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandPool = graphicsPool;
	commandBufferInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VK_ASSERT(vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer), "Failed to allocate Command Buffer for transferring Texture2D to Device")

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	transitionImageLayoutCmd(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer);
	copyBufferToImageCmd(width, height, stagingBuffer, textureImage, commandBuffer);
	transitionImageLayoutCmd(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	// Submit command buffer

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VkFence fence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device, &fenceInfo, nullptr, &fence);

	vkQueueSubmit(graphicsQueues[0], 1, &submitInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device, fence, nullptr);
	vkFreeCommandBuffers(device, graphicsPool, 1, &commandBuffer);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	// Create Texture Image View

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = textureImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VK_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &textureImageView), "Failed to create Texture2D view!");

	// Create Texture Sampler

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VK_ASSERT(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler), "Failed to create Texture2D sampler!")

	// Update descriptor set
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(VulkanUBO);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

	VkWriteDescriptorSet descriptorWrites[2] = {{}, {}};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
}

void VulkanCTX::ReleaseTexture()
{
	vkDestroySampler(device, textureSampler, nullptr);
	vkDestroyImageView(device, textureImageView, nullptr);
	vkDestroyImage(device, textureImage, nullptr);
	vkFreeMemory(device, textureMemory, nullptr);
}

void VulkanCTX::UpdateUniform(VulkanUBO newUBO)
{
	void *mappedData;
	vkMapMemory(device, uniformBufferMemory, 0, sizeof(newUBO), 0, &mappedData);
	memcpy(mappedData, &newUBO, sizeof(newUBO));
	vkUnmapMemory(device, uniformBufferMemory);
}
