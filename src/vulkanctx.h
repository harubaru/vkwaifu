#pragma once

#define GLFW_INCLUDE_VULKAN
#include <cstdlib>
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#define VK_ASSERT(x, msg) if (x != VK_SUCCESS) { std::cerr << "vulkan dieded: " << msg << std::endl; std::abort(); }
#define VK_FATAL(x, msg) if (x) { std::cerr << "vulkan dieded hard: " << msg << std::endl; std::abort(); }

#ifndef SURFACE_FORMAT
#define SURFACE_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#endif

#ifndef SURFACE_COLORSPACE
#define SURFACE_COLORSPACE VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
#endif

#ifndef PRESENT_MODE
#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
#endif

struct VulkanUBO {
	float time;
};

class VulkanCTX {
public:
	VulkanCTX() { ResetCache(); }
	virtual ~VulkanCTX() {}

	bool Setup(int width, int height);  // initializes vulkanctx - false on failure
	bool Resize(); // resizes swapchain - false on failure
	void Release(); // destroys vulkanctx
	void ResetCache(); // clears internal cache
	void Present(); // presents to screen
	void Update(); // update swapchain
	void ClearCurrentImage();

	void SetupGraphics(uint32_t width, uint32_t height); // Sets up Material
	void DrawGraphics(); // Draws material on quad

	void SetupTexture(uint8_t *data, uint32_t width, uint32_t height);
	void ReleaseTexture();

	void UpdateUniform(VulkanUBO newUBO);

	inline int ShouldClose() { return glfwWindowShouldClose(window); }
	inline void PollEvents() { glfwPollEvents(); }
	inline GLFWwindow *getWindow() { return window; }

	inline VkCommandBuffer getCurrentCommandBuffer() { return presentCommandBuffer[currentImage]; }
	inline VkImage getCurrentImage() { return swapchainImages[currentImage]; }

protected:

	// VulkanRenderer {
	VkInstance instance;
	VkDevice device;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDev;

	std::vector<VkFramebuffer> framebuffers; // Rendertargets
	VkRenderPass renderPass;		 // Global Renderpass
	// }

	// Material {
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	// };

	// CommandList {
	std::vector<VkQueue> graphicsQueues;
	VkCommandPool graphicsPool;
	uint32_t graphicsQueueFamily;
	// }

	// CommandList {
	std::vector<VkQueue> transferQueues;
	VkCommandPool transferPool;
	uint32_t transferQueueFamily;
	// }
	
	// Presenter {
	GLFWwindow *window;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkCommandBuffer> presentCommandBuffer;
	uint32_t currentImage, imageIndex;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkExtent2D swapExtent;

	std::vector<VkFence> fences; 
	std::vector<VkFence> inFlightFences; // in flight
	std::vector<VkSemaphore> acquireSemaphores; // available
	std::vector<VkSemaphore> presentSemaphores; // finished
	// }

	// Texture2D {
	VkImage textureImage;
	VkDeviceMemory textureMemory; // TODO: This is VERY bad! Use an allocator.
	VkImageView textureImageView;
	VkSampler textureSampler;
	// }
};
