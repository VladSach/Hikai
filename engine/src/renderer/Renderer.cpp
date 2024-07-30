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

void Renderer::init(const Window *window)
{
    LOG_INFO("Initializing Vulkan Renderer");

    window_ = window;

    context->init();

    createSurface();
    swapchain.init(surface, {window_->getWidth(), window_->getHeight()});

    createSyncObjects();

    createDepthResources();
    // FIX: temp
    gui.init(window);
    uiRenderPass = gui.uiRenderPass;
    createRenderPass();

    hk::ubo::init();

    createGraphicsPipeline();
    createFramebuffers();

    commandBuffer = context->graphics().createCommandBuffer();

    EventSystem::instance()->subscribe(hk::EVENT_WINDOW_RESIZE, resize, this);

    // FIX: temp
    model = hk::loader::loadModel("assets/models/smooth_vase.obj");
    model->populateBuffers();

    texture = hk::loader::loadImage(
        "assets/textures/prototype/PNG/Dark/texture_01.png");
}

void Renderer::deinit()
{
    VkDevice device = context->device();

    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = nullptr;
    }

    model->deinit();

    depthImage.deinit();

    pipeline.deinit();

    vkDestroyRenderPass(device, uiRenderPass, nullptr);
    vkDestroyRenderPass(device, sceneRenderPass, nullptr);

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

    u32 imageIndex;
    err = swapchain.acquireNextImage(acquireSemaphore, imageIndex);

    vkResetFences(context->device(), 1, &inFlightFence);
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin Command Buffer");

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
        viewport.width = static_cast<float>(swapchain.extent().width);
        viewport.height = static_cast<float>(swapchain.extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swapchain.extent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        model->bind(commandBuffer);
        texture->bind();
        getGlobalDescriptorWriter()->updateSet(*getGlobalDescriptorSet());

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline.layout(), 0, 1,
                                getGlobalDescriptorSet(), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, model->indexCount(), 1, 0, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffer);

    VkClearValue uiClearValue = {};
    uiClearValue.color = { 0.f, 0.f, 0.f, 1.f };

    VkRenderPassBeginInfo uiBeginInfo = {};
    uiBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    uiBeginInfo.renderPass = uiRenderPass;
    uiBeginInfo.renderArea.offset = { 0, 0 };
    uiBeginInfo.renderArea.extent = swapchain.extent();
    uiBeginInfo.framebuffer = uiFrameBuffers[imageIndex];
    uiBeginInfo.clearValueCount = 1;
    uiBeginInfo.pClearValues = &uiClearValue;

    vkCmdBeginRenderPass(commandBuffer, &uiBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    {
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
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
}

void Renderer::createGraphicsPipeline()
{
    VkResult err;
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
        getGlobalDescriptorSetLayout(),
    };
    builder.setLayout(descriptorSetsLayouts);

    builder.setDepthStencil();
    builder.setRenderInfo(swapchain.format(), depthImage.format());

    pipeline = builder.build(device, sceneRenderPass);

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
