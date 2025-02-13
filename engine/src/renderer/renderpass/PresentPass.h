#ifndef HK_PRESENT_PASS_H
#define HK_PRESENT_PASS_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Descriptors.h"

#include "utils/containers/hkvector.h"

namespace hk {

class PresentPass {
public:
    void init(hk::Swapchain *swapchain = nullptr);
    void deinit();

    void render(const hk::Image &source,
                VkCommandBuffer cmd, u32 idx,
                hk::DescriptorAllocator *alloc);

private:
    void loadShaders();
    void createSampler();
    void createFramebuffers();
    void createRenderPass();
    void createPipeline();

    void blit(const hk::Image &source, VkCommandBuffer cmd, u32 idx);
    void copy(const hk::Image &source, VkCommandBuffer cmd, u32 idx);
    void pass(const hk::Image &source, VkCommandBuffer cmd, u32 idx,
              hk::DescriptorAllocator *alloc);

private:
    hk::Pipeline pipeline_;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    hk::vector<VkFramebuffer> framebuffers_;

    u32 hndl_vertex_;
    u32 hndl_pixel_;

    VkDescriptorSetLayout set_layout_;

    VkSampler sampler_;

    // Configs
    b8 can_copy_;
    VkFormat color_format_;
    VkExtent2D size_;

    // Convenience
    VkDevice device_;
    hk::Swapchain *swapchain_;
};

}

#endif // HK_PRESENT_PASS_H
