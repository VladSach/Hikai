#ifndef HK_RENDERER_BACKEND_VULKAN_H
#define HK_RENDERER_BACKEND_VULKAN_H

#include "Backend.h"
#include "vendor/vulkan/vulkan.h"

#include "utils/containers/hkvector.h"
#include "math/math.h"

class BackendVulkan final : public Backend {
public:
    BackendVulkan(const Window &window)
        : window_(window) {}

    void init();
    void deinit();

    void draw();

    void cleanupSwapchain();

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
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffer();
    void createSyncObjects();

    VkShaderModule createShaderModule(const hk::vector<u32>& code);

    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void updateUniformBuffer(u32 currentImage);

    // FIX: temp
    struct UniformBuffer {
        hkm::vec2f resolution;
        f32 time;
    };
    UniformBuffer ubuffer;
    void setUniformBuffer(const hkm::vec2f &res, f32 time);

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
    VkExtent2D scExtent;
    hk::vector<VkImage> scImages;
    hk::vector<VkImageView> scImageViews;
    hk::vector<VkFramebuffer> scFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    hk::vector<VkBuffer> uniformBuffers;
    hk::vector<VkDeviceMemory> uniformBuffersMemory;
    hk::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    hk::vector<VkDescriptorSet> descriptorSets;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    VkSemaphore acquireSemaphore;
    VkSemaphore submitSemaphore;
    VkFence inFlightFence;

    b8 debugUtils = false;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    const Window &window_;
};

#endif // HK_RENDERER_BACKEND_VULKAN_H
