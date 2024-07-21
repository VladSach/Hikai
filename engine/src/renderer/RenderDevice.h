#ifndef HK_RENDER_DEVICE_H
#define HK_RENDER_DEVICE_H

#include "platform/Window.h"
#include "vendor/vulkan/vulkan.h"

#include "utils/containers/hkvector.h"

class RenderDevice {
public:
    ~RenderDevice() { deinit(); }

    void init(const Window *window);
    void deinit();

    void draw();

    void cleanupSwapchain();

    VkShaderModule createShaderModule(const hk::vector<u32> &code);

    void submitImmCmd(
        const std::function<void(VkCommandBuffer cmd)> &&func) const;

public:
    VkDevice logical() const { return device; }
    // FIX: remove access to physical device
    VkPhysicalDevice physical() const { return physicalDevice; }
    u32 getFrameBufferCount() { return FRAMEBUFFERS; }

private:
    const Window *window_;

    static constexpr u32 FRAMEBUFFERS = 2;

    // struct FrameData {
    //     VkSemaphore swapchainSemaphore, renderSemaphore;
    //     VkFence inFlightFence; // rendering fence
    // } frames[MAX_FRAMES_IN_FLIGHT];

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

    VkFence immCommandFence_ = VK_NULL_HANDLE;
    VkCommandPool immCommandPool_     = VK_NULL_HANDLE;
    VkCommandBuffer immCommandBuffer_ = VK_NULL_HANDLE;

    VkCommandPool commandPool     = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore submitSemaphore  = VK_NULL_HANDLE;
    VkFence inFlightFence        = VK_NULL_HANDLE;

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
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    void createSyncObjects();

public:
    struct BufferDesc {
        VkDeviceSize size;
        VkBufferUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

    struct DeviceBuffer {
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
    };

    DeviceBuffer createBuffer(const BufferDesc &desc) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                    VkDeviceSize size) const;


    struct ImageDesc {
        u32 width, height;
        VkFormat format;
        VkImageTiling tiling;
        VkImageUsageFlags usage;
        VkMemoryPropertyFlags properties;
    };

    struct DeviceImage {
        VkImage image;
        VkDeviceMemory imageMemory;
    };

    DeviceImage createImage(const ImageDesc &desc) const;

    void transitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

    void copyBufferToImage(VkBuffer buffer, VkImage image,
                           u32 width, u32 height);
};

namespace hk { RenderDevice *device(); }

#endif // HK_RENDER_DEVICE_H
