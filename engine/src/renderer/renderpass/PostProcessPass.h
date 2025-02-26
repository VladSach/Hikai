#ifndef HK_POST_PROCESS_PASS_H
#define HK_POST_PROCESS_PASS_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Descriptors.h"
#include "utils/containers/hkvector.h"

namespace hk {

class PostProcessPass {
public:
    void init(hk::Swapchain *swapchain);
    void deinit();

    void render(const hk::Image &source, VkCommandBuffer cmd, u32 idx,
                hk::DescriptorAllocator *alloc);

private:
    void loadShaders();
    void createSampler();
    void createFramebuffers();
    void createRenderPass();
    void createPipeline();

public:
    hk::Pipeline pipeline_;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

    hk::Image color_;

    VkDescriptorSetLayout set_layout_;

    u32 hndl_vertex_;
    u32 hndl_pixel_;

    VkSampler sampler_;

    // Configs
    VkFormat color_format_;
    VkExtent2D size_;

    // Convenience
    VkDevice device_;
    hk::Swapchain *swapchain_;
};

}

#endif // HK_POST_PROCESS_PASS_H
