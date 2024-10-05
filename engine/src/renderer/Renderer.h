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

#include "object/Model.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { deinit(); }

    void init(const Window *window);
    void deinit();

    void draw();

    static void resize(const hk::EventContext &size, void *listener);
    void recreateSwapchain();

    // FIX: temp
    constexpr GUI& ui() { return gui; }
    HKAPI void toggleUIMode();
    void addUIInfo();

private:
    const Window *window_;

    hk::VulkanContext *context = hk::context();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    hk::Swapchain swapchain;

    b8 resized = false;

    hk::DescriptorAllocator frameDescriptors;
    VkDescriptorSetLayout sceneDescriptorLayout;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    hk::Image depthImage;

    VkRenderPass presentRenderPass;
    hk::vector<VkFramebuffer> framebuffers;

    GUI gui;
    VkRenderPass uiRenderPass = VK_NULL_HANDLE;
    hk::vector<VkFramebuffer> uiFrameBuffers;
    b8 viewport = false;

    VkSemaphore acquireSemaphore = VK_NULL_HANDLE;
    VkSemaphore submitSemaphore  = VK_NULL_HANDLE;
    VkFence inFlightFence        = VK_NULL_HANDLE;

    // FIX: temp
    hk::Pipeline offscreenPipeline;
    VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
    VkFramebuffer offscreenFrameBuffer = VK_NULL_HANDLE;
    VkImage offscreenImage;
    VkImageView offscreenImageView;
    VkDeviceMemory offscreenMemory = VK_NULL_HANDLE;

    VkSampler samplerLinear;
    VkSampler samplerNearest;

    u32 hndlDefaultVS;
    u32 hndlDefaultPS;
    u32 hndlNormalsPS;
    u32 hndlTexturePS;

    hk::Pipeline gridPipeline;
    u32 hndlGridVS;
    u32 hndlGridPS;

    u32 curShaderVS;
    u32 curShaderPS;

    // FIX: temp
    hk::vector<hk::Model*> models;
    hk::vector<hk::RenderMaterial*> savebuff;

private:
    void createSurface();
    void createFramebuffers();
    void createDepthResources();
    void createSyncObjects();

    void createPresentRenderPass();
    void createOffscreenRenderPass();
    void createOffscreenPipeline();

    void createGridPipeline();

    void loadShaders();
    void createSamplers();
};

#endif // HK_RENDER_DEVICE_H
