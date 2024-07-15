#ifndef HK_RENDERER_BACKEND_VULKAN_H
#define HK_RENDERER_BACKEND_VULKAN_H

#include "platform/Window.h"
#include "vendor/vulkan/vulkan.h"

#include "utils/containers/hkvector.h"

#include "renderer/Buffer.h"
#include "renderer/Vertex.h"

class BackendVulkan {
public:
    BackendVulkan(const Window &window)
        : window_(window) {}

    ~BackendVulkan() { deinit(); }

    void init();
    void deinit();

    void draw();

    void cleanupSwapchain();

    VkShaderModule createShaderModule(const hk::vector<u32> &code);

    // FIX: temp
    struct UniformBuffer {
        hkm::vec2f resolution;
        f32 time;
    } ubuffer;
    void setUniformBuffer(const hkm::vec2f &res, f32 time);

private:
    const Window &window_;

    static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameData {
        VkSemaphore swapchainSemaphore, renderSemaphore;
        VkFence inFlightFence; // rendering fence
    } frames[MAX_FRAMES_IN_FLIGHT];

    VkInstance instance = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice         device         = VK_NULL_HANDLE;

    u32 graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    u32 computeFamily  = VK_QUEUE_FAMILY_IGNORED;
    u32 transferFamily = VK_QUEUE_FAMILY_IGNORED;
    u32 presentFamily  = VK_QUEUE_FAMILY_IGNORED;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue  = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    VkQueue presentQueue  = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat scImageFormat;
    VkExtent2D scExtent;
    hk::vector<VkImage> scImages;
    hk::vector<VkImageView> scImageViews;
    hk::vector<VkFramebuffer> scFramebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkPipeline graphicsPipeline     = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    VkCommandPool commandPool     = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore submitSemaphore  = VK_NULL_HANDLE;
    VkFence inFlightFence        = VK_NULL_HANDLE;

    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer uniformBuffer;

    VkDescriptorPool descriptorPool;
    hk::vector<VkDescriptorSet> descriptorSets;

    /* TODO: add debug functionality
     * https://github.com/KhronosGroup/Vulkan-Samples/tree/main/
       samples/extensions/debug_utils
     */
    b8 debugUtils = false;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

private:
    void createInstance();
    void createSurface();
    void createPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffer();
    void createSyncObjects();

    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};

#endif // HK_RENDERER_BACKEND_VULKAN_H
