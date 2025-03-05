#include "PostProcessPass.h"

#include "renderer/VulkanContext.h"
#include "resources/AssetManager.h"

namespace hk {

void PostProcessPass::init(hk::Swapchain *swapchain)
{
    LOG_TRACE("Creating Post Process RenderPass");

    device_ = hk::context()->device();
    swapchain_ = swapchain;

    color_format_ = swapchain_->format();
    size_ = swapchain_->extent();

    loadShaders();
    createSampler();
    createRenderPass();
    createPipeline();
    createFramebuffers();
}

void PostProcessPass::deinit()
{
    LOG_TRACE("Destroying Post Process RenderPass");

    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    framebuffer_ = VK_NULL_HANDLE;

    pipeline_.deinit();

    color_.deinit();

    vkDestroyRenderPass(device_, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;

    vkDestroySampler(device_, sampler_,  nullptr);
    sampler_ = VK_NULL_HANDLE;

    hndl_vertex_ = 0;
    hndl_pixel_ = 0;

    swapchain_ = nullptr;
    device_ = VK_NULL_HANDLE;

    color_format_ = VK_FORMAT_UNDEFINED;
    size_ = {};
}

void PostProcessPass::render(const hk::Image &source,
                             VkCommandBuffer cmd, u32 idx,
                             hk::DescriptorAllocator *alloc)
{
    hk::DescriptorWriter writer;

    VkClearValue post_process_clear[1] = {};
    post_process_clear[0].color = { 0.f, 0.f, 0.f, 1.f };

    VkRenderPassBeginInfo post_process_info = {};
    post_process_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    post_process_info.renderPass = render_pass_;
    post_process_info.renderArea.offset = { 0, 0 };
    post_process_info.renderArea.extent = size_;
    post_process_info.framebuffer = framebuffer_;
    post_process_info.clearValueCount = 1;
    post_process_info.pClearValues = post_process_clear;

    VkDescriptorSet present_set = alloc->allocate(set_layout_);

    writer.writeImage(2, source.view(), sampler_,
                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.updateSet(present_set);

    vkCmdBeginRenderPass(cmd, &post_process_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline_.layout(), 0, 1,
                            &present_set, 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
}

void PostProcessPass::loadShaders()
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

    desc.path = path + "Present.vert.hlsl";
    hndl_vertex_ = hk::assets()->load(desc.path, &desc);

    desc.type = ShaderType::Pixel;

    desc.path = path + "PostProcess.frag.hlsl";
    hndl_pixel_ = hk::assets()->load(desc.path, &desc);
}

void PostProcessPass::createSampler()
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
    hk::debug::setName(sampler_, "Post Process Pass Sampler");
}

void PostProcessPass::createFramebuffers()
{
    VkResult err;

    color_.init({
        "Post Process Color Attachment",
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

    VkImageView attachments[] = {
        color_.view(),
    };

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = render_pass_;
    info.width = size_.width;
    info.height = size_.height;
    info.layers = 1;
    info.attachmentCount = 1;
    info.pAttachments = attachments;

    err = vkCreateFramebuffer(device_, &info, nullptr, &framebuffer_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    hk::debug::setName(framebuffer_, "Post Process Framebuffer");
}

void PostProcessPass::createRenderPass()
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
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

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
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    err = vkCreateRenderPass(device_, &info, nullptr, &render_pass_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
    hk::debug::setName(render_pass_, "Post Process RenderPass");
}

void PostProcessPass::createPipeline()
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

    hk::DescriptorLayout *post_process_layout =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(2,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );
    set_layout_ = post_process_layout->layout();
    hk::debug::setName(set_layout_, "Post Process Descriptor Layout");

    hk::vector<VkDescriptorSetLayout> set_layouts = { set_layout_ };
    builder.setLayout(set_layouts);

    builder.setDepthStencil(VK_FALSE, VK_COMPARE_OP_NEVER);
    builder.setRenderInfo({ color_format_ }, VK_FORMAT_UNDEFINED);

    pipeline_ = builder.build(device_, render_pass_);
    hk::debug::setName(pipeline_.handle(), "Post Process Pipeline");
}

}
