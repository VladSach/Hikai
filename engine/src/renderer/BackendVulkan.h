#ifndef HK_RENDERER_BACKEND_VULKAN_H
#define HK_RENDERER_BACKEND_VULKAN_H

#include "Backend.h"
#include "utils/containers/hkvector.h"
#include "vendor/vulkan/vulkan.h"

class BackendVulkan final : public Backend {
public:
    void init(const Window &window);
    void deinit();

    void draw();

    void cleanupSwapchain();

    void createInstance();
    void createSurface(const Window &window);
    void createPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    void createSyncObjects();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    /* TODO: add debug functionality
     * https://github.com/KhronosGroup/Vulkan-Samples/tree/main/
       samples/extensions/debug_utils
     */

private:
    VkInstance instance             = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device                 = VK_NULL_HANDLE;

    u32 graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    u32 computeFamily  = VK_QUEUE_FAMILY_IGNORED;
    u32 transferFamily = VK_QUEUE_FAMILY_IGNORED;
    u32 presentFamily  = VK_QUEUE_FAMILY_IGNORED;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue  = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkQueue presentQueue  = VK_NULL_HANDLE;

    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    VkSwapchainKHR swapchain;
    VkFormat scImageFormat;
    hk::vector<VkImage> scImages;
    hk::vector<VkImageView> scImageViews;
    hk::vector<VkFramebuffer> scFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore acquireSemaphore;
    VkSemaphore submitSemaphore;
    VkFence inFlightFence;

    VkDescriptorPool descriptorPool;

    b8 debugUtils = false;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};

#endif // HK_RENDERER_BACKEND_VULKAN_H
