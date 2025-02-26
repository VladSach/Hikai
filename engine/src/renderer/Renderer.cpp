#include "Renderer.h"

#include "platform/platform.h"
#include "renderer/VertexLayout.h"
#include "renderer/vkwrappers/vkdebug.h"
#include "resources/AssetManager.h"

// TODO: change with hikai implementation
#include <algorithm>
#include <set>

// FIX: temp
static ModelToWorld modelToWorld;

void Renderer::init(const Window *window)
{
    LOG_INFO("Initializing Vulkan Renderer");

    window_ = window;

    hk::context()->init();

    device_ = hk::context()->device();
    physical_ = hk::context()->physical();

    swapchain_.init(window_);
    swapchain_.recreate(
        { window_->width(), window_->height() },
        { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
        VK_PRESENT_MODE_MAILBOX_KHR);

    hk::vector<hk::DescriptorAllocator::TypeSize> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
    };

    global_desc_alloc.init(100, sizes);

    hk::DescriptorLayout *descriptor_layout =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(0,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1,
                    // PERF: right now no need to change to ssbo but maybe later
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );

    global_desc_layout = descriptor_layout->layout();
    hk::debug::setName(global_desc_layout, "Per Frame Descriptor Set Layout");

    createFrameResources();

    // FIX: temp
    STATIC_ASSERT(sizeof(SceneData) % 16 == 0,
                  "Uniform Buffer padding is incorrect");

    hk::Buffer::BufferDesc desc;
    desc.type = hk::Buffer::Type::UNIFORM_BUFFER;
    desc.usage = hk::Buffer::Usage::NONE;
    desc.property = hk::Buffer::Property::CPU_ACESSIBLE;
    desc.size = 1; // FIX: temp, make depend on framebuffers size
    desc.stride = sizeof(SceneData);
    frame_data_buffer.init(desc);

    desc.stride = sizeof(LightSources);
    lights_buffer.init(desc);
    // end ubo

    use_ui_ = true;

    loadShaders();

    offscreen_.init(&swapchain_, global_desc_layout);
    post_process_.init(&swapchain_);
    present_.init(&swapchain_);
    ui_.init(window_, &swapchain_);

    createGridPipeline();

    createSamplers();

    hk::event::subscribe(hk::event::EVENT_WINDOW_RESIZE, resize, this);
}

void Renderer::deinit()
{
    vkDeviceWaitIdle(device_);

    vkDestroySampler(device_, samplerLinear,  nullptr);
    vkDestroySampler(device_, samplerNearest, nullptr);

    offscreen_.deinit();
    post_process_.deinit();
    present_.deinit();
    ui_.deinit();

    // FIX: temp
    frame_data_buffer.deinit();

    for (auto frame : frames_) {
        vkDestroySemaphore(device_, frame.acquire_semaphore, nullptr);
        vkDestroySemaphore(device_, frame.submit_semaphore, nullptr);
        vkDestroyFence(device_, frame.in_flight_fence, nullptr);
    }

    swapchain_.deinit();

    hk::context()->deinit();
}

void Renderer::draw(hk::DrawContext &ctx)
{
    VkResult err;

    FrameData frame = frames_[current_frame_];

    vkWaitForFences(device_, 1, &frame.in_flight_fence, VK_TRUE, UINT64_MAX);

    u32 image_idx = 0;
    err = swapchain_.acquireNextImage(frame.acquire_semaphore, image_idx);

    vkResetFences(device_, 1, &frame.in_flight_fence);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        resize({static_cast<u16>(window_->width()),
                static_cast<u16>(window_->height())}, this);
        return;
    } else if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Failed to acquire Swapchain image");
    }

    vkResetCommandBuffer(frame.cmd, 0);

    frame.descriptor_alloc.clear();

    VkDescriptorSet global_desc_set =
        frame.descriptor_alloc.allocate(global_desc_layout);

    hk::DescriptorWriter writer;
    writer.writeBuffer(0, frame_data_buffer.buffer(),
                       sizeof(SceneData), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    writer.writeBuffer(1, lights_buffer.buffer(),
                       sizeof(LightSources), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(frame.cmd, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin Command Buffer");

    offscreen_.begin(frame.cmd, image_idx);

        writer.updateSet(global_desc_set);

        // FIX: global desc set should be bound once before anything else

        /* FIX: make 4 descriptor sets and change bound
         * Per-frame    0
         * Per-pass     1
         * Per-material 2
         * Per-instance 3
         */

        for (auto &object : ctx.objects) {
            hk::MaterialInstance &mat = object.material;

            vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mat.pipeline->handle());

            // per-frame descriptor (0)
            vkCmdBindDescriptorSets(frame.cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    mat.pipeline->layout(), 0, 1,
                                    &global_desc_set, 0, nullptr);

            object.bind(frame.cmd);

            // per-material descriptor (2)
            vkCmdBindDescriptorSets(frame.cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    mat.pipeline->layout(), 1, 1,
                                    &mat.materialSet, 0, nullptr);

            u32 numInstances = u32(object.instances.size());

            for (u32 meshIndex = 0; meshIndex < object.instances.size(); ++meshIndex) {
                modelToWorld.transform = object.instances.at(meshIndex);

                // per-instance descriptor (3)
                vkCmdPushConstants(frame.cmd, mat.pipeline->layout(),
                                    VK_SHADER_STAGE_VERTEX_BIT, 0,
                                    sizeof(modelToWorld), &modelToWorld);

                vkCmdDrawIndexed(frame.cmd,
                                    object.indexBuffer.size(),
                                    numInstances,
                                    0,
                                    0,
                                    meshIndex);
            }

        }

        // vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //                   offscreen_.pipeline.handle());

        // Draw grid shader
        vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            gridPipeline.handle());
        vkCmdDraw(frame.cmd, 4, 1, 0, 0);

    offscreen_.end(frame.cmd);


    // VkResult f = vkGetFenceStatus(hk::context()->device(), frame.in_flight_fence);
    // LOG_DEBUG("Current Index: ", imageIndex,
    //           "- Fence Status: ", f == VK_SUCCESS ? "SIGNALED" : "UNSIGNALED");

    /* There are 3 cases
     * No UI pass, offscreen image sampled into swapchain in Present pass, UI pass omitted
     * UI pass w viweport, offscreen image used in UI pass, then UI pass rendered to swapchain, Present omitted
     * UI pass wo viewport, offscreen image used in UI pass, then UI pass renderer to swapchain, Present omitted
     */

    // Wait for previous render passes to complete
    /* https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-
     * Examples#first-draw-writes-to-a-color-attachment-second-draw-samples-
     * from-that-color-image-in-the-fragment-shader
     */
    VkImageMemoryBarrier bar = {};
    bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bar.image = offscreen_.color_.image();
    bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bar.subresourceRange.baseMipLevel = 0;
    bar.subresourceRange.levelCount = 1;
    bar.subresourceRange.baseArrayLayer = 0;
    bar.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(frame.cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    post_process_.render(offscreen_.color_,
                         frame.cmd, image_idx,
                         &frame.descriptor_alloc);

    bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    bar.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    bar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    bar.image = post_process_.color_.image();
    bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bar.subresourceRange.baseMipLevel = 0;
    bar.subresourceRange.levelCount = 1;
    bar.subresourceRange.baseArrayLayer = 0;
    bar.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(frame.cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &bar);

    if (use_ui_) {
        ui_.render(frame.cmd, image_idx);
    } else {
        present_.render(post_process_.color_,
                        frame.cmd, image_idx,
                        &frame.descriptor_alloc);
    }

    err = vkEndCommandBuffer(frame.cmd);
    ALWAYS_ASSERT(!err, "Failed to end Command Buffer");

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.cmd;
    submitInfo.pSignalSemaphores = &frame.submit_semaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &frame.acquire_semaphore;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = &waitStage;

    VkQueue graphicsQueue = hk::context()->graphics().handle();
    err = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frame.in_flight_fence);
    ALWAYS_ASSERT(!err, "Failed to submit Vulkan draw Command Buffer");

    err = swapchain_.present(image_idx, frame.submit_semaphore);

    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR || resized) {
        resize({static_cast<u16>(window_->width()),
                static_cast<u16>(window_->height())}, this);
    } else if (err != VK_SUCCESS) {
        LOG_ERROR("Failed to present Swapchain image");
    }

    current_frame_ = (current_frame_ + 1) % max_frames_;
}

void Renderer::createFrameResources()
{
    VkResult err;

    frames_.resize(max_frames_);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    hk::vector<hk::DescriptorAllocator::TypeSize> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
    };

    for (u32 i = 0; i < max_frames_; ++i) {
        std::string idx = std::to_string(i);

        frames_[i].cmd = hk::context()->graphics().createCommandBuffer();
        hk::debug::setName(frames_[i].cmd, "Command Buffer Frame #" + idx);

        err = vkCreateSemaphore(device_, &semaphore_info, nullptr, &frames_[i].acquire_semaphore);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");
        hk::debug::setName(frames_[i].acquire_semaphore, "Acquire Semaphore Frame #" + idx);

        err = vkCreateSemaphore(device_, &semaphore_info, nullptr, &frames_[i].submit_semaphore);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Semaphore");
        hk::debug::setName(frames_[i].submit_semaphore, "Submit Semaphore Frame #" + idx);

        err = vkCreateFence(device_, &fence_info, nullptr, &frames_[i].in_flight_fence);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Fence");
        hk::debug::setName(frames_[i].in_flight_fence, "In Flight Fence Frame #" + idx);

        frames_[i].descriptor_alloc.init(10, sizes);
    }
}

void Renderer::createGridPipeline()
{
    hk::PipelineBuilder builder;

    VkShaderModule vs = hk::assets()->getShader(hndlGridVS).module;
    VkShaderModule ps = hk::assets()->getShader(hndlGridPS).module;
    builder.setShader(ShaderType::Vertex, vs);
    builder.setShader(ShaderType::Pixel, ps);

    builder.setVertexLayout(0, 0);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          VK_CULL_MODE_NONE,
                          VK_FRONT_FACE_CLOCKWISE);
    builder.setMultisampling();
    builder.setColorBlend();

    hk::vector<VkDescriptorSetLayout> descriptorSetsLayouts = {
        global_desc_layout,
    };
    builder.setLayout(descriptorSetsLayouts);

    builder.setPushConstants(sizeof(modelToWorld));

    builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.setRenderInfo(swapchain_.format(), offscreen_.depth_.format());

    gridPipeline = builder.build(device_, offscreen_.render_pass_);
    hk::debug::setName(gridPipeline.handle(), "Grid Pipeline");
}

void Renderer::loadShaders()
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

    desc.path = path + "Default.vert.hlsl";
    hndlDefaultVS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlDefaultVS, [this](){
        resized = true;
    });

    desc.path = path + "Grid.vert.hlsl";
    hndlGridVS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlGridVS, [this](){
        createGridPipeline();
    });

    desc.type = ShaderType::Pixel;

    desc.path = path + "Default.frag.hlsl";
    hndlDefaultPS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlDefaultPS, [this](){
        resized = true;
    });

    desc.path = path + "Normals.frag.hlsl";
    hndlNormalsPS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlNormalsPS, [this](){
        resized = true;
    });

    desc.path = path + "Texture.frag.hlsl";
    hndlTexturePS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlTexturePS, [this](){
        resized = true;
    });

    desc.path = path + "PBR.frag.hlsl";
    hndlPBR = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlPBR, [this](){
        resized = true;
    });

    desc.path = path + "Grid.frag.hlsl";
    hndlGridPS = hk::assets()->load(desc.path, &desc);
    hk::assets()->attachCallback(hndlGridPS, [this](){
        createGridPipeline();
    });

    curShaderVS = hndlDefaultVS;
    curShaderPS = hndlDefaultPS;
}

void Renderer::createSamplers()
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

    vkCreateSampler(device_, &samplerInfo, nullptr, &samplerLinear);
    hk::debug::setName(samplerLinear, "Default Linear Sampler");

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(device_, &samplerInfo, nullptr, &samplerNearest);
    hk::debug::setName(samplerNearest, "Default Nearest Sampler");
}

void Renderer::resize(const hk::event::EventContext &size, void *listener)
{
    Renderer *renderer = reinterpret_cast<Renderer*>(listener);

    vkDeviceWaitIdle(renderer->device_);

    renderer->swapchain_.recreate({size.u32[0], size.u32[1]});

    renderer->ui_.deinit();
    renderer->present_.deinit();
    renderer->post_process_.deinit();
    renderer->offscreen_.deinit();

    renderer->offscreen_.init(&renderer->swapchain_, renderer->global_desc_layout);
    renderer->post_process_.init(&renderer->swapchain_);
    renderer->present_.init(&renderer->swapchain_);
    renderer->ui_.init(renderer->window_, &renderer->swapchain_);

    renderer->resized = false;
}
