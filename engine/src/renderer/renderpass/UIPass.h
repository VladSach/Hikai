#ifndef HK_UI_PASS_H
#define HK_UI_PASS_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Descriptors.h"

#include "hkstl/containers/hkvector.h"

namespace hk {

class UIPass {
public:
    void init(const Window *window, hk::Swapchain *swapchain);
    void deinit();

    void render(VkCommandBuffer cmd, u32 idx);

private:
    void loadShaders();
    void createSampler();
    void createFramebuffers();
    void createRenderPass();
    void createPipeline();

private:
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    hk::vector<VkFramebuffer> framebuffers_;

    // Configs
    VkFormat color_format_;
    VkExtent2D size_;

    // Convenience
    VkDevice device_;
    hk::Swapchain *swapchain_;
};

}

#endif // HK_UI_PASS_H
