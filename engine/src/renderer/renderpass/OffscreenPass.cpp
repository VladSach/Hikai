#include "OffscreenPass.h"

#include "renderer/VulkanContext.h"
#include "resources/AssetManager.h"

namespace hk {

void OffscreenPass::init(hk::Swapchain *swapchain, VkDescriptorSetLayout layout)
{
    LOG_TRACE("Creating Deferred RenderPass");

    device_ = hk::context()->device();
    swapchain_ = swapchain;
    set_layout_ = layout;

    // color_format_ = swapchain_->format();
    color_format_ = VK_FORMAT_R16G16B16A16_SFLOAT; // HDR
    depth_format_ = VK_FORMAT_D32_SFLOAT;
    size_ = swapchain_->extent();

    createRenderPass();
    loadShaders();
    createPipeline();
    createFramebuffers();
}

void OffscreenPass::deinit()
{
    LOG_TRACE("Destroying Deferred RenderPass");

    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    framebuffer_ = VK_NULL_HANDLE;

    albedo_.deinit();
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

    VkClearValue clear_values[6] = {};
    clear_values[0].color = { 0.f, 0.f, 0.f, 0.f };
    clear_values[1].color = { 0.f, 0.f, 0.f, 0.f };
    clear_values[2].color = { 0.f, 0.f, 0.f, 0.f };
    clear_values[3].color = { 0.f, 0.f, 0.f, 0.f };
    clear_values[4].depthStencil = { 0.f, 0 };
    clear_values[5].color = { 0.f, 0.f, 0.f, 0.f };

    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = render_pass_;
    begin_info.renderArea.offset = { 0, 0 };
    begin_info.renderArea.extent = size_;
    begin_info.framebuffer = framebuffer_;
    begin_info.clearValueCount = 6;
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

    position_.init({
        "Deferred Position attachment",
        hk::Image::Usage::SAMPLED      |
        // hk::Image::Usage::TRANSFER_SRC |
        // hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT |
        hk::Image::Usage::INPUT_ATTACHMENT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });

    normal_.init({
        "Deferred Normal attachment",
        hk::Image::Usage::SAMPLED      |
        // hk::Image::Usage::TRANSFER_SRC |
        // hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT |
        hk::Image::Usage::INPUT_ATTACHMENT,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });

    albedo_.init({
        "Deferred Albedo attachment",
        hk::Image::Usage::SAMPLED      |
        // hk::Image::Usage::TRANSFER_SRC |
        // hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT |
        hk::Image::Usage::INPUT_ATTACHMENT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });

    material_.init({
        "Deferred Material attachment",
        hk::Image::Usage::SAMPLED      |
        // hk::Image::Usage::TRANSFER_SRC |
        // hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT |
        hk::Image::Usage::INPUT_ATTACHMENT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });

    depth_.init({
        "Deferred Depth attachment",
        hk::Image::Usage::DEPTH_STENCIL_ATTACHMENT |
        hk::Image::Usage::INPUT_ATTACHMENT,
        depth_format_,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        size_.width, size_.height, 1,
    });

    color_.init({
        "Deferred Final Color attachment",
        hk::Image::Usage::SAMPLED      |
        hk::Image::Usage::TRANSFER_SRC |
        hk::Image::Usage::TRANSFER_DST |
        hk::Image::Usage::COLOR_ATTACHMENT,
        color_format_,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        size_.width, size_.height, 1,
    });

    VkImageView attachments[] = {
        position_.view(),
        normal_.view(),
        albedo_.view(),
        material_.view(),
        depth_.view(),
        color_.view()
    };

    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = render_pass_;
    info.width = size_.width;
    info.height = size_.height;
    info.layers = 1;
    info.attachmentCount = 6;
    info.pAttachments = attachments;

    err = vkCreateFramebuffer(device_, &info, nullptr, &framebuffer_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    hk::debug::setName(framebuffer_, "Deferred Framebuffer");
}

void OffscreenPass::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription attachments[6] = {};

    // Position Attachment
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Normal Attachment
    attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Albedo Attachment
    attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Materials Attachment
    attachments[3].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment
    attachments[4].format = depth_format_;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Final Color Attachment
    attachments[5].format = color_format_;
    attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[5].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[5].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference geometry_attachments[] = {
        { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, // position
        { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, // normal
        { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, // albedo
        { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, // material
        { 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }, // final color
    };
    VkAttachmentReference depth_attachment = {};
    depth_attachment.attachment = 4;
    depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference light_attachments[] = {
        { 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // position
        { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // normal
        { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // albedo
        { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // material
        { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, // depth
    };

    VkAttachmentReference light_attachment = {};
    light_attachment.attachment = 5;
    light_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[2] = {};

    // Geometry pass
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 5;
    subpasses[0].pColorAttachments = geometry_attachments;
    subpasses[0].pDepthStencilAttachment = &depth_attachment;

    // Light pass
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &light_attachment;
    // subpasses[1].pDepthStencilAttachment = &depth_attachment;
    subpasses[1].inputAttachmentCount = 5;
    subpasses[1].pInputAttachments = light_attachments;

    VkSubpassDependency dependencies[4] = {};

    // This makes sure that writes to the depth image are done before we try to write to it again
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dependencyFlags = 0;

    dependencies[2].srcSubpass = 0; // geometry pass
    dependencies[2].dstSubpass = 1; // light pass
    dependencies[2].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[3].srcSubpass = 1;
    dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[3].srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 6;
    info.pAttachments = attachments;
    info.subpassCount = 2;
    info.pSubpasses = subpasses;
    info.dependencyCount = 4;
    info.pDependencies = dependencies;

    err = vkCreateRenderPass(device_, &info, nullptr, &render_pass_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
    hk::debug::setName(render_pass_, "Deferred RenderPass");
}

void OffscreenPass::loadShaders()
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
    desc.path = path + "PBR.frag.hlsl";
    hndl_pixel_ = hk::assets()->load(desc.path, &desc);

    // FIX: temp
    desc.path = path + "Deferred.frag.hlsl";
    hk::assets()->load(desc.path, &desc);
}

void OffscreenPass::createPipeline()
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

    hk::DescriptorLayout *light_layout =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(0,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(1,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(2,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(3,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .addBinding(4,
                    VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .build()
    );

    VkDescriptorSetLayout scene_layout = set_layout_;

    set_layout_ = light_layout->layout();
    hk::debug::setName(set_layout_, "Light Descriptor Layout");

    hk::vector<VkDescriptorSetLayout> set_layouts = {
        scene_layout,
        set_layout_
    };
    builder.setLayout(set_layouts);

    builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    formats_ = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R32_SFLOAT,
    };
    builder.setRenderInfo({ color_format_ }, depth_format_);

    pipeline_ = builder.build(device_, render_pass_, 1);
    hk::debug::setName(pipeline_.handle(), "Light Pipeline");
}

}
