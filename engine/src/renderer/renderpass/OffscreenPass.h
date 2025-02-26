#ifndef HK_OFFSCREEN_PASS_H
#define HK_OFFSCREEN_PASS_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "utils/containers/hkvector.h"

namespace hk {

class OffscreenPass {
public:
    void init(hk::Swapchain *swapchain, VkDescriptorSetLayout layout);
    void deinit();

    // void render(VkCommandBuffer cmd, u32 idx);
    void begin(VkCommandBuffer cmd, u32 idx);
    void end(VkCommandBuffer cmd);

    void setShaders(u32 vertex, u32 pixel);

private:
    void createFramebuffers();
    void createRenderPass();
    void createPipeline();

// FIX: temp public
public:
    // hk::Pipeline pipeline_;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    // FIX: shouln't have multilple framebuffers?
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

    hk::Image color_;
    hk::Image depth_;

    VkDescriptorSetLayout set_layout_;

    // u32 hndl_vertex;
    // u32 hndl_pixel;

    // Configs
    VkFormat color_format_;
    VkFormat depth_format_;
    VkExtent2D size_;

    // Convenience
    VkDevice device_;
    hk::Swapchain *swapchain_;
};

}

#endif // HK_OFFSCREEN_PASS_H
