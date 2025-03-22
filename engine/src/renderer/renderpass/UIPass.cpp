#include "UIPass.h"

#include "renderer/VulkanContext.h"
#include "renderer/ui/imguiwrapper.h"

#include "resources/AssetManager.h"

namespace hk {

void UIPass::init(const Window *window, hk::Swapchain *swapchain)
{
    LOG_TRACE("Creating UI RenderPass");

    device_ = hk::context()->device();
    swapchain_ = swapchain;

    color_format_ = swapchain_->format();
    size_ = swapchain_->extent();

    createRenderPass();
    createFramebuffers();

    // TODO: imgui should be initialized and deinitilized inside pass
    hk::imgui::init(window, render_pass_);
}

void UIPass::deinit()
{
    LOG_TRACE("Destroying UI RenderPass");

    hk::imgui::deinit();

    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;

    swapchain_ = nullptr;
    device_ = VK_NULL_HANDLE;

    color_format_ = VK_FORMAT_UNDEFINED;
    size_ = {};
}

void UIPass::render(VkCommandBuffer cmd, u32 idx)
{
    VkClearValue ui_clear[1] = {};
    ui_clear[0].color = { 0.f, 0.f, 0.f, 1.f };

    VkRenderPassBeginInfo ui_info = {};
    ui_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    ui_info.renderPass = render_pass_;
    ui_info.renderArea.offset = { 0, 0 };
    ui_info.renderArea.extent = size_;
    ui_info.framebuffer = framebuffers_[idx];
    ui_info.clearValueCount = 1;
    ui_info.pClearValues = ui_clear;

    // hk::debug::label::begin(cmd, "UI Render Pass", {1.f, 0.f, 0.f, 1.f});

    vkCmdBeginRenderPass(cmd, &ui_info, VK_SUBPASS_CONTENTS_INLINE);

    hk::imgui::draw(cmd);

    vkCmdEndRenderPass(cmd);

    // hk::debug::label::end(cmd);
}

void UIPass::createFramebuffers()
{
    VkResult err;

    const hk::vector<VkImageView> &views = swapchain_->views();
    framebuffers_.resize(views.size());

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = render_pass_;
    info.attachmentCount = 1;
    info.width = size_.width;
    info.height = size_.height;
    info.layers = 1;

    for (u32 i = 0; i < views.size(); i++) {
        info.pAttachments = &views[i];

        err = vkCreateFramebuffer(device_, &info, nullptr, &framebuffers_[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
        hk::debug::setName(framebuffers_[i],
                           "UI Framebuffer #" + std::to_string(i));
    }
}

void UIPass::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription attachments[1] = {};

    // Color Attachment
    attachments[0].format = color_format_;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;

    err = vkCreateRenderPass(device_, &info, nullptr, &render_pass_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
    hk::debug::setName(render_pass_, "UI RenderPass");
}

}
