#ifndef HK_RENDER_DEVICE_H
#define HK_RENDER_DEVICE_H

#include "platform/Window.h"
#include "vendor/vulkan/vulkan.h"

#include "renderer/VulkanContext.h"
#include "renderer/Descriptors.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Image.h"

#include "renderer/gui/gui.h"

#include "core/EventSystem.h"
#include "utils/containers/hkvector.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { deinit(); }

    void init(const Window *window);
    void deinit();

    void draw();

    VkShaderModule createShaderModule(const hk::vector<u32> &code);

    static void resize(hk::EventContext size, void *listener);

    // FIX: temp
    constexpr GUI& ui() { return gui; }
    HKAPI void toggleUIMode();

private:
    const Window *window_;

    hk::VulkanContext *context = hk::context();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    hk::Swapchain swapchain;

    hk::DescriptorAllocator frameDescriptors;
    VkDescriptorSetLayout sceneDescriptorLayout;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkRenderPass sceneRenderPass = VK_NULL_HANDLE;
    hk::Image depthImage;
    hk::vector<VkFramebuffer> framebuffers;
    hk::Pipeline pipeline;

    // FIX: temp
    hk::Pipeline offscreenPipeline;
    VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
    VkFramebuffer offscreenFrameBuffer = VK_NULL_HANDLE;
    VkImage offscreenImage;
    VkImageView offscreenImageView;
    VkDeviceMemory offscreenMemory = VK_NULL_HANDLE;

    GUI gui;
    VkRenderPass uiRenderPass = VK_NULL_HANDLE;
    hk::vector<VkFramebuffer> uiFrameBuffers;
    b8 viewport = false;

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore submitSemaphore  = VK_NULL_HANDLE;
    VkFence inFlightFence        = VK_NULL_HANDLE;
private:
    void createSurface();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createDepthResources();
    void createSyncObjects();
};

#endif // HK_RENDER_DEVICE_H
