#include "Renderer.h"

#include "platform/platform.h"
#include "renderer/ShaderManager.h"
#include "renderer/VertexLayout.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

// TODO: change with hikai implementation
#include <algorithm>
#include <set>

// FIX: temp
#include "renderer/UBManager.h"
#include "object/Model.h"
#include "utils/loaders/ModelLoader.h"
static hk::Model *model;

#include "utils/loaders/ImageLoader.h"
static hk::Image *texture;
static VkSampler sampler;

void Renderer::toggleUIMode()
{
    viewport = !viewport;

    if (viewport) {
        gui.setViewportMode(offscreenImageView);
    } else {
        gui.setOverlayMode();
    }
}

void Renderer::init(const Window *window)
{
    LOG_INFO("Initializing Vulkan Renderer");

    window_ = window;

    context->init();

    createSurface();
    swapchain.init(surface, {window_->getWidth(), window_->getHeight()});

    hk::vector<hk::DescriptorAllocator::TypeSize> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
    };
    frameDescriptors.init(10, sizes);

    hk::DescriptorLayout *sceneDataDescriptorLayout =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(0,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_FRAGMENT_BIT)
        .build()
    );
    sceneDescriptorLayout = sceneDataDescriptorLayout->layout();

    createSyncObjects();

    createDepthResources();

    // FIX: temp
    gui.init(window);
    uiRenderPass = gui.uiRenderPass;

    createRenderPass();

    hk::ubo::init();

    createGraphicsPipeline();
    createFramebuffers();

    // FIX: temp
    toggleUIMode();

    commandBuffer = context->graphics().createCommandBuffer();

    EventSystem::instance()->subscribe(hk::EVENT_WINDOW_RESIZE, resize, this);

    // FIX: temp
    model = hk::loader::loadModel("assets/models/smooth_vase.obj");
    model->populateBuffers();

    texture = hk::loader::loadImage(
        "assets/textures/prototype/PNG/Dark/texture_01.png");

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(hk::context()->physical(), &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(context->device(), &samplerInfo, nullptr, &sampler);
}

void Renderer::deinit()
{
    VkDevice device = context->device();

    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = nullptr;
    }

    model->deinit();
    if (sampler)
        vkDestroySampler(device, sampler, nullptr);

    depthImage.deinit();

    pipeline.deinit();

    vkDestroyRenderPass(device, uiRenderPass, nullptr);
    // vkDestroyRenderPass(device, sceneRenderPass, nullptr);

    hk::ubo::deinit();

    vkDestroySemaphore(device, acquireSemaphore, nullptr);
    vkDestroySemaphore(device, submitSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

    swapchain.deinit();

    vkDestroySurfaceKHR(context->instance(), surface, nullptr);

    hk::context()->deinit();
}

void Renderer::draw()
{
    VkResult err;

    vkWaitForFences(context->device(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    frameDescriptors.clear();

    VkDescriptorSet sceneDataDescriptor =
        frameDescriptors.allocate(sceneDescriptorLayout);

    hk::DescriptorWriter writer;
    writer.writeBuffer(0, hk::ubo::getFrameData().buffer(),
                       sizeof(hk::ubo::SceneData), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    writer.writeImage(1, texture->view(), sampler, texture->layout(),
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    u32 imageIndex;
    err = swapchain.acquireNextImage(acquireSemaphore, imageIndex);

    vkResetFences(context->device(), 1, &inFlightFence);
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin Command Buffer");

    if (!viewport) {
        VkClearValue clearValue[2] = {};
        clearValue[0].color = { 0.f, 0.f, 0.f, 1.f };
        clearValue[1].depthStencil = { 0.f, 0 };

        VkRenderPassBeginInfo rpBeginInfo = {};
        rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBeginInfo.renderPass = sceneRenderPass;
        rpBeginInfo.renderArea.offset = { 0, 0 };
        rpBeginInfo.renderArea.extent = swapchain.extent();
        rpBeginInfo.framebuffer = framebuffers[imageIndex];
        rpBeginInfo.clearValueCount = 2;
        rpBeginInfo.pClearValues = clearValue;

        vkCmdBeginRenderPass(commandBuffer, &rpBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeline.handle());

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<f32>(swapchain.extent().width);
            viewport.height = static_cast<f32>(swapchain.extent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = swapchain.extent();
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            model->bind(commandBuffer);

            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipeline.layout(), 0, 1,
                                    &sceneDataDescriptor, 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, model->indexCount(), 1, 0, 0, 0);

        }
        vkCmdEndRenderPass(commandBuffer);
    } else {
        // FIX: temp
        VkClearValue offClearValue[2] = {};
        offClearValue[0].color = { 0.f, 0.f, 0.f, 1.f };
        offClearValue[1].depthStencil = { 0.f, 0 };

        VkRenderPassBeginInfo offBeginInfo = {};
        offBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        offBeginInfo.renderPass = offscreenRenderPass;
        offBeginInfo.renderArea.offset = { 0, 0 };
        offBeginInfo.renderArea.extent = swapchain.extent();
        offBeginInfo.framebuffer = offscreenFrameBuffer;
        offBeginInfo.clearValueCount = 2;
        offBeginInfo.pClearValues = offClearValue;

        vkCmdBeginRenderPass(commandBuffer, &offBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              offscreenPipeline.handle());

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<f32>(swapchain.extent().width);
            viewport.height = static_cast<f32>(swapchain.extent().height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor = {};
            scissor.offset = {0, 0};
            scissor.extent = swapchain.extent();
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            model->bind(commandBuffer);

            writer.updateSet(sceneDataDescriptor);

            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    offscreenPipeline.layout(), 0, 1,
                                    &sceneDataDescriptor, 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, model->indexCount(), 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    }

    VkClearValue uiClearValue = {};
    uiClearValue.color = { 0.f, 0.f, 0.f, 1.f };

    VkRenderPassBeginInfo uiBeginInfo = {};
    uiBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    uiBeginInfo.renderPass = uiRenderPass;
    uiBeginInfo.renderArea.offset = { 0, 0 };
    uiBeginInfo.renderArea.extent = swapchain.extent();
    uiBeginInfo.framebuffer = uiFrameBuffers[imageIndex];
    uiBeginInfo.clearValueCount = 2;
    uiBeginInfo.pClearValues = &uiClearValue;

    vkCmdBeginRenderPass(commandBuffer, &uiBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    {
        // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //                   pipeline.handle());
        gui.draw(commandBuffer);
    }
    vkCmdEndRenderPass(commandBuffer);

    err = vkEndCommandBuffer(commandBuffer);
    ALWAYS_ASSERT(!err, "Failed to end Command Buffer");

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.pSignalSemaphores = &submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &waitStage;

    VkQueue graphicsQueue = hk::context()->graphics().handle();
    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
    ALWAYS_ASSERT(!err, "Failed to submit Vulkan draw Command Buffer");

    err = swapchain.present(imageIndex, submitSemaphore);
}

void Renderer::createSurface()
{
    VkResult err;

    // TODO: add other platforms
#ifdef HKWINDOWS
    const WinWindow *win = static_cast<const WinWindow *>(window_);
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = win->getHWnd();
    surfaceInfo.hinstance = win->getHInstance();
    err = vkCreateWin32SurfaceKHR(context->instance(),
                                  &surfaceInfo, 0, &surface);
#endif // HKWINDOWS

    ALWAYS_ASSERT(!err, "Failed to create Vulkan Surface");
}

void Renderer::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchain.format();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthImage.format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[] = {
        colorAttachment, depthAttachment
    };

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
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    err = vkCreateRenderPass(context->device(), &renderPassInfo,
                             nullptr, &sceneRenderPass);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");

    // FIX: temp
    VkAttachmentDescription offscreenColorAttachment = {};
    offscreenColorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    offscreenColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    offscreenColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    offscreenColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    offscreenColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    offscreenColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    offscreenColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    offscreenColorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference offscreenColorAttachmentRef = {};
    offscreenColorAttachmentRef.attachment = 0;
    offscreenColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription offscreenDepthAttachment = {};
    offscreenDepthAttachment.format = depthImage.format();
    offscreenDepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    offscreenDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    offscreenDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    offscreenDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    offscreenDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    offscreenDepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    offscreenDepthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference offscreenDepthAttachmentRef = {};
    offscreenDepthAttachmentRef.attachment = 1;
    offscreenDepthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription offscreenSubpass = {};
    offscreenSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    offscreenSubpass.colorAttachmentCount = 1;
    offscreenSubpass.pColorAttachments = &offscreenColorAttachmentRef;
    offscreenSubpass.pDepthStencilAttachment = &offscreenDepthAttachmentRef;

    VkAttachmentDescription offAttachments[] = {
        offscreenColorAttachment, offscreenDepthAttachment
    };

    VkSubpassDependency offDependency = {};
    offDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    offDependency.dstSubpass = 0;
    offDependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    offDependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    offDependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo offRenderPassInfo = {};
    offRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    offRenderPassInfo.attachmentCount = 2;
    offRenderPassInfo.pAttachments = offAttachments;
    offRenderPassInfo.subpassCount = 1;
    offRenderPassInfo.pSubpasses = &offscreenSubpass;
    offRenderPassInfo.dependencyCount = 1;
    offRenderPassInfo.pDependencies = &offDependency;

    err = vkCreateRenderPass(context->device(), &offRenderPassInfo,
                             nullptr, &offscreenRenderPass);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
}

void Renderer::createGraphicsPipeline()
{
    VkDevice device = context->device();

    hk::PipelineBuilder builder;

    const std::string path = "assets\\shaders\\";

    hk::dxc::ShaderDesc desc;
    desc.path = path + "VisibleNormalsVS.hlsl";
    desc.entry = "main";
    desc.type = ShaderType::Vertex;
    desc.model = ShaderModel::SM_6_0;
    desc.ir = ShaderIR::SPIRV;
#ifdef HKDEBUG
    desc.debug = true;
#else
    desc.debug = false;
#endif

    auto vertShaderCode = hk::dxc::loadShader(desc);

    desc.path = path + "VisibleNormalsPS.hlsl";
    desc.type = ShaderType::Pixel;
    auto fragShaderCode = hk::dxc::loadShader(desc);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    builder.setShader(ShaderType::Vertex, vertShaderModule);
    builder.setShader(ShaderType::Pixel, fragShaderModule);

    hk::vector<hk::Format> layout = {
        // position
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // normal
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // texture coordinates
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC2 | hk::Format::B32,
    };
    builder.setVertexLayout(sizeof(Vertex), layout);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);
    builder.setMultisampling();
    builder.setColorBlend();

    hk::vector<VkDescriptorSetLayout> descriptorSetsLayouts = {
        sceneDescriptorLayout,
    };
    builder.setLayout(descriptorSetsLayouts);

    builder.setDepthStencil();
    builder.setRenderInfo(swapchain.format(), depthImage.format());

    // pipeline = builder.build(device, sceneRenderPass);
    offscreenPipeline = builder.build(device, offscreenRenderPass);

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Renderer::createFramebuffers()
{
    VkResult err;

    const hk::vector<VkImageView> &views = swapchain.views();

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = sceneRenderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.width = swapchain.extent().width;
    framebufferInfo.height = swapchain.extent().height;
    framebufferInfo.layers = 1;

    framebuffers.resize(views.size());
    for (u32 i = 0; i < views.size(); i++) {
        VkImageView attachments[] = {
            views[i],
            depthImage.view()
        };

        framebufferInfo.pAttachments = attachments;

        err = vkCreateFramebuffer(context->device(), &framebufferInfo,
                                  nullptr, &framebuffers[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    }

    VkFramebufferCreateInfo uiFramebufferInfo = {};
    uiFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    uiFramebufferInfo.renderPass = uiRenderPass;
    uiFramebufferInfo.attachmentCount = 1;
    uiFramebufferInfo.width = swapchain.extent().width;
    uiFramebufferInfo.height = swapchain.extent().height;
    uiFramebufferInfo.layers = 1;
    uiFrameBuffers.resize(views.size());
    for (u32 i = 0; i < views.size(); i++) {
        VkImageView attachments[] = {
            views[i],
        };

        uiFramebufferInfo.pAttachments = attachments;

        err = vkCreateFramebuffer(context->device(), &uiFramebufferInfo,
                                  nullptr, &uiFrameBuffers[i]);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
    }

    // FIX: temp
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchain.extent().width;
    imageInfo.extent.height = swapchain.extent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = vkCreateImage(context->device(), &imageInfo, nullptr, &offscreenImage);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context->device(), offscreenImage, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context->physical(), &memProperties);

    u32 memIndex = 0;
    for (; memIndex < memProperties.memoryTypeCount; ++memIndex) {
        if ((memRequirements.memoryTypeBits & (1 << memIndex)) &&
            (memProperties.memoryTypes[memIndex].propertyFlags &
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memIndex;

    err = vkAllocateMemory(context->device(), &allocInfo, nullptr, &offscreenMemory);

    vkBindImageMemory(context->device(), offscreenImage, offscreenMemory, 0);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = offscreenImage;
    viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(context->device(), &viewInfo, nullptr, &offscreenImageView);

    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = offscreenImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    // VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //
    // hk::context()->submitImmCmd([&](VkCommandBuffer cmd) {
    //     vkCmdPipelineBarrier(
    //         cmd,
    //         sourceStage, destinationStage,
    //         0,
    //         0, nullptr,
    //         0, nullptr,
    //         1, &barrier
    //     );
    // });

    VkFramebufferCreateInfo offframe = {};
    offframe.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    offframe.renderPass = offscreenRenderPass;
    offframe.attachmentCount = 2;
    offframe.width = swapchain.extent().width;
    offframe.height = swapchain.extent().height;
    offframe.layers = 1;
    VkImageView offattachments[] = {
        offscreenImageView,
        depthImage.view()
    };

    offframe.pAttachments = offattachments;

    err = vkCreateFramebuffer(context->device(), &offframe,
                              nullptr, &offscreenFrameBuffer);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Framebuffer");
}

void Renderer::createSyncObjects()
{
    VkResult err;
    VkDevice device = context->device();

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquireSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    err = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &submitSemaphore);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    err = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");
}

void Renderer::createDepthResources()
{
    depthImage.init({
        hk::Image::Usage::DEPTH_STENCIL_ATTACHMENT,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        swapchain.extent().width, swapchain.extent().height, 1,
    });
}

VkShaderModule Renderer::createShaderModule(const hk::vector<u32> &code)
{
    VkResult err;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(u32);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    err = vkCreateShaderModule(context->device(), &createInfo,
                               nullptr, &shaderModule);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");

    return shaderModule;
}

void Renderer::resize(hk::EventContext size, void *listener)
{
    Renderer *renderer = reinterpret_cast<Renderer*>(listener);

    renderer->swapchain.init(renderer->surface, {size.u32[0], size.u32[1]});
    renderer->createDepthResources();
    renderer->createFramebuffers();
}
