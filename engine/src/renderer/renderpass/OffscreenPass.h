#ifndef HK_OFFSCREEN_PASS_H
#define HK_OFFSCREEN_PASS_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/vkwrappers/Buffer.h"
#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Descriptors.h"

#include "hkstl/containers/hkvector.h"

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
    void loadShaders();
    void createPipeline(VkDescriptorSetLayout scene_layout);

// FIX: temp public
public:
    VkRenderPass render_pass_ = VK_NULL_HANDLE;

    // Gbuffer
    hk::Image position_;
    hk::Image normal_;
    hk::Image albedo_;
    hk::Image material_; // Material Properties i.e metallic, roughness

    hk::Image depth_;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;


    // TODO:
    // albedo   3d [0, 1]    r8g8b8unorm
    // material 4d [0, 1]    r8g8b8a8unorm | metallic, roughness, ao, ? | maybe bitshift together to reduce size
    // normal   4d [-1, +1]  r16g16b16a16snorm | rg from map, ba from vertex | packed in octahedrons
    // emmision 3d [0, +inf] r16g16b16a16float
    // metadata 1d r32uint | entity id, material id, whatever is needed
    // No need for position
    // https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/

    // Geometry pass
    // FIX: rename, it's not geometry but rather global pipeline
    // it's single use is to bind global desc set
    hk::Pipeline geometry_pipeline_;

    // Light pass
    hk::Image color_;
    hk::Pipeline pipeline_;
    hk::DescriptorLayout set_layout_;

    u32 hndl_vertex_;
    u32 hndl_pixel_;

    // Configs
    VkFormat color_format_;
    VkFormat depth_format_;
    hk::vector<VkFormat> formats_;
    VkExtent2D size_;

    // Convenience
    VkDevice device_;
    hk::Swapchain *swapchain_;
};

}

#endif // HK_OFFSCREEN_PASS_H
