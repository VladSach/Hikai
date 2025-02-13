#include "OffscreenPass.h"

#include "renderer/VulkanContext.h"
#include "resources/AssetManager.h"

namespace hk {

void OffscreenPass::init(hk::Swapchain *swapchain)
{
    LOG_TRACE("Creating Offscreen RenderPass");

    device_ = hk::context()->device();
    swapchain_ = swapchain;

    color_format_ = swapchain_->format();
    depth_format_ = VK_FORMAT_D32_SFLOAT;
    size_ = swapchain_->extent();

    createRenderPass();
    createFramebuffers();
}

void OffscreenPass::deinit()
{
    LOG_TRACE("Destroying Offscreen RenderPass");

    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    framebuffer_ = VK_NULL_HANDLE;

    color_.deinit();
    depth_.deinit();

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;

    swapchain_ = nullptr;
    device_ = VK_NULL_HANDLE;

    color_format_ = VK_FORMAT_UNDEFINED;
    size_ = {};
}

void OffscreenPass::begin(VkCommandBuffer cmd, u32 idx)
{
    (void)idx; // FIX: temp

    VkClearValue clear_values[2] = {};
    clear_values[0].color = { 0.f, 0.f, 0.f, 1.f };
    clear_values[1].depthStencil = { 0.f, 0 };

    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass_;
    begin_info.renderArea.offset = { 0, 0 };
    begin_info.renderArea.extent = size_;
    begin_info.framebuffer = framebuffer_;
    begin_info.clearValueCount = 2;
    begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(size_.width);
    viewport.height = static_cast<f32>(size_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = size_;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void OffscreenPass::end(VkCommandBuffer cmd)
{
    vkCmdEndRenderPass(cmd);
}

void OffscreenPass::createFramebuffers()
{
    VkResult err;

    color_.init({
        hk::Image::Usage::SAMPLED      |
        hk::Image::Usage::TRANSFER_SRC |
        hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT,
        color_format_,
        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });
    color_.setName("Offscreen Color");

    depth_.init({
        hk::Image::Usage::DEPTH_STENCIL_ATTACHMENT,
        depth_format_,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        size_.width, size_.height, 1,
    });
    depth_.setName("Offscreen Depth");

    VkImageView attachments[] = {
        color_.view(),
        depth_.view()
    };

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = render_pass_;
    info.width = size_.width;
    info.height = size_.height;
    info.layers = 1;
    info.attachmentCount = 2;
    info.pAttachments = attachments;

    err = vkCreateFramebuffer(device_, &info, nullptr, &framebuffer_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    hk::debug::setName(framebuffer_, "Offscreen Framebuffer");
}

void OffscreenPass::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription attachments[2] = {};

    // Color Attachment
    attachments[0].format = color_format_;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth Attachment
    attachments[1].format = depth_format_;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment = {};
    depth_attachment.attachment = 1;
    depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;
    subpass.pDepthStencilAttachment = &depth_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    err = vkCreateRenderPass(device_, &info, nullptr, &render_pass_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
    hk::debug::setName(render_pass_, "Offscreen RenderPass");
}

void OffscreenPass::createPipeline()
{
    // hk::PipelineBuilder builder;
    //
    // builder.setShader(ShaderType::Vertex, hk::assets()->getShader(hndl_vertex).module);
    // builder.setShader(ShaderType::Pixel, hk::assets()->getShader(hndl_pixel).module);
    //
    // hk::vector<hk::bitflag<Format>> layout = {
    //     // position
    //     hk::Format::SIGNED | hk::Format::FLOAT |
    //     hk::Format::VEC3 | hk::Format::B32,
    //
    //     // normal
    //     hk::Format::SIGNED | hk::Format::FLOAT |
    //     hk::Format::VEC3 | hk::Format::B32,
    //
    //     // texture coordinates
    //     hk::Format::SIGNED | hk::Format::FLOAT |
    //     hk::Format::VEC2 | hk::Format::B32,
    // };
    // builder.setVertexLayout(sizeof(Vertex), layout);
    //
    // builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // builder.setRasterizer(VK_POLYGON_MODE_FILL,
    //                       VK_CULL_MODE_BACK_BIT,
    //                       VK_FRONT_FACE_CLOCKWISE);
    // builder.setMultisampling();
    // builder.setColorBlend();
    //
    // hk::vector<VkDescriptorSetLayout> descriptorSetsLayouts = {
    //     set_layout_
    // };
    // builder.setLayout(descriptorSetsLayouts);
    //
    // builder.setPushConstants(sizeof(modelToWorld));
    //
    // builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    // builder.setRenderInfo(color_format_, depth_format_);
    //
    // pipeline_ = builder.build(device_, render_pass_);
    // hk::debug::setName(pipeline_.handle(), "Offscreen Pipeline");
}

}
