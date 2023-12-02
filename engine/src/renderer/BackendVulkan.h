#ifndef HK_RENDERER_BACKEND_VULKAN_H
#define HK_RENDERER_BACKEND_VULKAN_H

#include "Backend.h"
#include "utils/containers/hkvector.h"
#include "vendor/vulkan/vulkan.h"

class BackendVulkan final : public Backend {
public:
    void init();
    void deinit();

private:
    VkInstance instance                     = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice         = VK_NULL_HANDLE;
    VkDevice device                         = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	u32 graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
	u32 computeFamily  = VK_QUEUE_FAMILY_IGNORED;
	u32 copyFamily     = VK_QUEUE_FAMILY_IGNORED;
	u32 videoFamily    = VK_QUEUE_FAMILY_IGNORED;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue computeQueue  = VK_NULL_HANDLE;
	VkQueue copyQueue     = VK_NULL_HANDLE;
	VkQueue videoQueue    = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool;

    VkSurfaceFormatKHR surfaceFormat;
    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    VkSurfaceKHR surface;
};

#endif // HK_RENDERER_BACKEND_VULKAN_H
