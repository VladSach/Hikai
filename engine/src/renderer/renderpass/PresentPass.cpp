#include "PresentPass.h"

#include "renderer/VulkanContext.h"
#include "resources/AssetManager.h"

namespace hk {

void PresentPass::init(hk::Swapchain *swapchain)
{
    LOG_TRACE("Creating Present RenderPass");

    ALWAYS_ASSERT(swapchain_ || swapchain,
                  "Can't create Present Pass without swapchain");

    device_ = hk::context()->device();
    if (swapchain) { swapchain_ = swapchain; }

    color_format_ = swapchain_->format();
    size_ = swapchain_->extent();

    can_copy_ = swapchain_->info().caps.supportedUsageFlags &
                VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (can_copy_) {
        // TODO: set swapchain images from UNDEFINED to PRESENT layout
    }

    loadShaders();
    createSampler();
    createRenderPass();
    createPipeline();

    createFramebuffers();
}

void PresentPass::deinit()
{
    LOG_TRACE("Destroying Present RenderPass");

    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }

    pipeline_.deinit();

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;

    vkDestroySampler(device_, sampler_,  nullptr);
    sampler_ = VK_NULL_HANDLE;

    hndl_vertex_ = 0;
    hndl_pixel_ = 0;

    swapchain_ = nullptr;
    device_ = VK_NULL_HANDLE;

    set_layout_ = VK_NULL_HANDLE;
    can_copy_ = false;
    color_format_ = VK_FORMAT_UNDEFINED;
    size_ = {};
}

void PresentPass::render(const hk::Image &source,
                         VkCommandBuffer cmd, u32 idx,
                         hk::DescriptorAllocator *alloc)
{
    // If swapchain DST is supported - copy or blit, depending on image size
    // If not - separate render pass

    // PERF: Probably don't branch mid frame
    // compare with func pointer or find other way to choose before rendering

    if (can_copy_) {
        if (size_.width  == source.width() && size_.height == source.height()) {
            copy(source, cmd, idx);
        } else {
            blit(source, cmd, idx);
        }
    } else {
        pass(source, cmd, idx, alloc);
    }
}

void PresentPass::copy(const hk::Image &source, VkCommandBuffer cmd, u32 idx)
{
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.mipLevel = 0;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.extent.width = size_.width;
    copyRegion.extent.height = size_.height;
    copyRegion.extent.depth = 1;

    hk::vector<VkImage> &scimages = swapchain_->images();

    VkImageMemoryBarrier bar = {};
    bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar.image = scimages[idx];
    bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bar.subresourceRange.baseMipLevel = 0;
    bar.subresourceRange.levelCount = 1;
    bar.subresourceRange.baseArrayLayer = 0;
    bar.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    bar.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.image = source.image();
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    vkCmdCopyImage(
        cmd,
        source.image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        scimages[idx],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion
    );

    bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bar.image = scimages[idx];
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.image = source.image();
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);
}


void PresentPass::blit(const hk::Image &source, VkCommandBuffer cmd, u32 idx)
{
    VkImageBlit blit_region = {};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.mipLevel = 0;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcOffsets[0].x = 0;
    blit_region.srcOffsets[0].y = 0;
    blit_region.srcOffsets[0].z = 0;
    blit_region.srcOffsets[1].x = source.width();
    blit_region.srcOffsets[1].y = source.height();
    blit_region.srcOffsets[1].z = 1;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.mipLevel = 0;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstOffsets[0].x = 0;
    blit_region.dstOffsets[0].y = 0;
    blit_region.dstOffsets[0].z = 0;
    blit_region.dstOffsets[1].x = size_.width;
    blit_region.dstOffsets[1].y = size_.height;
    blit_region.dstOffsets[1].z = 1;

    hk::vector<VkImage> &scimages = swapchain_->images();

    VkImageMemoryBarrier bar = {};
    bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bar.image = scimages[idx];
    bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bar.subresourceRange.baseMipLevel = 0;
    bar.subresourceRange.levelCount = 1;
    bar.subresourceRange.baseArrayLayer = 0;
    bar.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    bar.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.image = source.image();
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    vkCmdBlitImage(
        cmd,
        source.image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        scimages[idx],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit_region,
        VK_FILTER_NEAREST
    );

    bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    bar.image = scimages[idx];
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.image = source.image();
    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);
}

void PresentPass::pass(const hk::Image &source,
                       VkCommandBuffer cmd, u32 idx,
                       hk::DescriptorAllocator *alloc)
{
    hk::DescriptorWriter writer;

    VkClearValue present_clear[1] = {};
    present_clear[0].color = { 0.f, 0.f, 0.f, 1.f };

    VkRenderPassBeginInfo present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    present_info.renderPass = render_pass_;
    present_info.renderArea.offset = { 0, 0 };
    present_info.renderArea.extent = size_;
    present_info.framebuffer = framebuffers_[idx];
    present_info.clearValueCount = 1;
    present_info.pClearValues = present_clear;

    VkDescriptorSet present_set = alloc->allocate(set_layout_);

    // writer.clear();
    writer.writeImage(2, source.view(), sampler_, source.layout(),
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.updateSet(present_set);

    vkCmdBeginRenderPass(cmd, &present_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_.layout(), 0, 1,
                            &present_set, 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
}

void PresentPass::loadShaders()
{
    const std::string path = "..\\engine\\assets\\shaders\\";

    hk::dxc::ShaderDesc desc;
    desc.entry = "main";
    desc.type = ShaderType::Vertex;
    desc.model = ShaderModel::SM_6_0;
    desc.ir = ShaderIR::SPIRV;
#ifdef HKDEBUG
    desc.debug = true;
#else
    desc.debug = false;
#endif

    desc.path = path + "PresentVS.hlsl";
    hndl_vertex_ = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndl_vertex_, [this](){
        deinit();
        init();
    });

    desc.type = ShaderType::Pixel;

    desc.path = path + "PresentPS.hlsl";
    hndl_pixel_ = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndl_pixel_, [this](){
        deinit();
        init();
    });
}

void PresentPass::createSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    const auto info = hk::context()->physicalInfo();

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = info.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_);
    hk::debug::setName(sampler_, "Present Pass Sampler");
}

void PresentPass::createFramebuffers()
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
                           "Present Framebuffer #" + std::to_string(i));
    }
}

void PresentPass::createRenderPass()
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
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Present Render Pass");
    hk::debug::setName(render_pass_, "Present RenderPass");
}

void PresentPass::createPipeline()
{
    hk::PipelineBuilder builder;

    VkShaderModule vs = hk::assets()->getShader(hndl_vertex_).module;
    VkShaderModule ps = hk::assets()->getShader(hndl_pixel_).module;
    builder.setShader(ShaderType::Vertex, vs);
    builder.setShader(ShaderType::Pixel, ps);

    builder.setVertexLayout(0, 0);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          VK_CULL_MODE_FRONT_BIT,
                          VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisampling();
    builder.setColorBlend();

    hk::DescriptorLayout *present_layout =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(2,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );
    set_layout_ = present_layout->layout();
    hk::debug::setName(set_layout_, "Present Descriptor Layout");

    hk::vector<VkDescriptorSetLayout> set_layouts = { set_layout_ };
    builder.setLayout(set_layouts);

    builder.setDepthStencil(VK_FALSE, VK_COMPARE_OP_NEVER);
    builder.setRenderInfo(color_format_, VK_FORMAT_UNDEFINED);

    pipeline_ = builder.build(device_, render_pass_);
    hk::debug::setName(pipeline_.handle(), "Present Pipeline");
}

}
