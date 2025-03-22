#include "Renderer.h"

#include "platform/platform.h"
#include "resources/AssetManager.h"

#include "renderer/VertexLayout.h"
#include "renderer/ui/debug_draw.h"

// FIX: temp
#include "renderer/ui/imguidebug.h"

// FIX: temp
static InstanceData instance_data;

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
        { VK_DESCRIPTOR_TYPE_SAMPLER, 12 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 6 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 6 },
    };
    global_desc_alloc.init(100, sizes);

    createSamplers();

    global_desc_layout.init(
        hk::DescriptorLayout::Builder()
        // Scene and other data
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)

        // Lights
        // PERF: right now no need to change to ssbo but maybe later
        .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)

        // Static Samplers
        .addBinding(2, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.nearest.repeat)
        .addBinding(3, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.nearest.mirror)
        .addBinding(4, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.nearest.clamp)
        .addBinding(5, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.nearest.border)

        .addBinding(6, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.linear.repeat)
        .addBinding(7, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.linear.mirror)
        .addBinding(8, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.linear.clamp)
        .addBinding(9, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.linear.border)

        .addBinding(10, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.anisotropic.repeat)
        .addBinding(11, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.anisotropic.mirror)
        .addBinding(12, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.anisotropic.clamp)
        .addBinding(13, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &samplers_.anisotropic.border)
        .build()
    );
    hk::debug::setName(global_desc_layout.handle(), "Per Frame Descriptor Set Layout");

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

    hk::debug::setName(frame_data_buffer.buffer(), "Uniform Buffer - Global");
    hk::debug::setName(lights_buffer.buffer(), "Uniform Buffer - Lights");
    // end ubo

    use_ui_ = true;

    loadShaders();

    offscreen_.init(&swapchain_, global_desc_layout.handle());
    post_process_.init(&swapchain_);
    present_.init(&swapchain_);
    ui_.init(window_, &swapchain_);

    hk::dd::init(global_desc_layout.handle(),
                 offscreen_.set_layout_.handle(), offscreen_.render_pass_);

    createGridPipeline();

    hk::event::subscribe(hk::event::EVENT_WINDOW_RESIZE, resize, this);
}

void Renderer::deinit()
{
    vkDeviceWaitIdle(device_);

    vkDestroySampler(device_, samplers_.linear.repeat, nullptr);
    vkDestroySampler(device_, samplers_.linear.mirror, nullptr);
    vkDestroySampler(device_, samplers_.linear.clamp,  nullptr);
    vkDestroySampler(device_, samplers_.linear.border, nullptr);

    vkDestroySampler(device_, samplers_.nearest.repeat, nullptr);
    vkDestroySampler(device_, samplers_.nearest.mirror, nullptr);
    vkDestroySampler(device_, samplers_.nearest.clamp,  nullptr);
    vkDestroySampler(device_, samplers_.nearest.border, nullptr);

    vkDestroySampler(device_, samplers_.anisotropic.repeat, nullptr);
    vkDestroySampler(device_, samplers_.anisotropic.mirror, nullptr);
    vkDestroySampler(device_, samplers_.anisotropic.clamp,  nullptr);
    vkDestroySampler(device_, samplers_.anisotropic.border, nullptr);

    offscreen_.deinit();
    post_process_.deinit();
    present_.deinit();
    ui_.deinit();

    gridPipeline.deinit();
    hk::dd::deinit();

    global_desc_layout.deinit();
    global_desc_alloc.deinit();

    // FIX: temp
    frame_data_buffer.deinit();
    lights_buffer.deinit();

    for (auto frame : frames_) {
        vkDestroySemaphore(device_, frame.acquire_semaphore, nullptr);
        vkDestroySemaphore(device_, frame.submit_semaphore, nullptr);
        vkDestroyFence(device_, frame.in_flight_fence, nullptr);
        frame.descriptor_alloc.deinit();
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
        hk::event::EventContext context;
        context.u32[0] = window_->width();
        context.u32[1] = window_->height();
        resize(context, this);
        return;
    } else if (err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Failed to acquire Swapchain image");
    }

    vkResetCommandBuffer(frame.cmd, 0);

    frame.descriptor_alloc.clear();

    VkDescriptorSet global_desc_set =
        frame.descriptor_alloc.allocate(global_desc_layout.handle());

    hk::DescriptorWriter writer;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    err = vkBeginCommandBuffer(frame.cmd, &beginInfo);
    ALWAYS_ASSERT(!err, "Failed to begin Command Buffer");

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<f32>(swapchain_.extent().width);
    viewport.height = static_cast<f32>(swapchain_.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_.extent();

    vkCmdSetViewport(frame.cmd, 0, 1, &viewport);
    vkCmdSetScissor(frame.cmd, 0, 1, &scissor);

    VkPipelineBindPoint bind_point_graphics = VK_PIPELINE_BIND_POINT_GRAPHICS;

    /* === Descriptor Sets ===
     * Per-frame    0
     * Per-pass     1
     * Per-material 2
     * Per-instance 3 (right now push constants)
     */

    offscreen_.begin(frame.cmd, image_idx);
        writer.writeBuffer(0, frame_data_buffer.buffer(),
                           sizeof(SceneData), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.writeBuffer(1, lights_buffer.buffer(),
                           sizeof(LightSources), 0,
                           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.updateSet(global_desc_set);

        offscreen_.geometry_pipeline_.bind(frame.cmd, bind_point_graphics);

        // per-frame descriptor (0)
        vkCmdBindDescriptorSets(frame.cmd,
                                bind_point_graphics,
                                offscreen_.geometry_pipeline_.layout(), 0, 1,
                                &global_desc_set, 0, nullptr);

        // Geometry Pass
        for (auto &object : ctx.objects) {
            hk::MaterialInstance &mat = object.material;

            mat.pipeline->bind(frame.cmd, bind_point_graphics);

            object.bind(frame.cmd);

            // per-material descriptor (2)
            vkCmdBindDescriptorSets(frame.cmd,
                                    bind_point_graphics,
                                    mat.pipeline->layout(), 2, 1,
                                    &mat.materialSet, 0, nullptr);

            u32 instance_count = object.instances.size();
            for (u32 mesh_idx = 0; mesh_idx < instance_count; ++mesh_idx) {
                instance_data.model_to_world = object.instances.at(mesh_idx);

                // per-instance descriptor (3)
                vkCmdPushConstants(frame.cmd, mat.pipeline->layout(),
                                   VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                                   sizeof(instance_data), &instance_data);

                vkCmdDrawIndexed(frame.cmd, object.indexBuffer.size(),
                                 instance_count, 0, 0, mesh_idx);
            }

        }

        // Light pass
        vkCmdNextSubpass(frame.cmd, VK_SUBPASS_CONTENTS_INLINE);

        // TODO: change all images layout to shader read

        // FIX: temp
        vkCmdBindDescriptorSets(frame.cmd,
                                bind_point_graphics,
                                offscreen_.pipeline_.layout(), 0, 1,
                                &global_desc_set, 0, nullptr);

        // per-pass descriptor (1)
        VkDescriptorSet light_set = global_desc_alloc.allocate(offscreen_.set_layout_.handle());
        writer.clear();
        writer.writeImage(0, offscreen_.position_.view(), VK_NULL_HANDLE,
                          // offscreen_.position_.layout(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        writer.writeImage(1, offscreen_.normal_.view(), VK_NULL_HANDLE,
                          // offscreen_.normal_.layout(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        writer.writeImage(2, offscreen_.albedo_.view(), VK_NULL_HANDLE,
                          // offscreen_.albedo_.layout(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        writer.writeImage(3, offscreen_.material_.view(), VK_NULL_HANDLE,
                          // offscreen_.material_.layout(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
        writer.writeImage(4, offscreen_.depth_.view(), VK_NULL_HANDLE,
                          // offscreen_.depth_.layout(),
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

        writer.updateSet(light_set);

        offscreen_.pipeline_.bind(frame.cmd, bind_point_graphics);

        vkCmdBindDescriptorSets(frame.cmd, bind_point_graphics,
                            offscreen_.pipeline_.layout(), 1, 1,
                            &light_set, 0, nullptr);

        vkCmdDraw(frame.cmd, 3, 1, 0, 0);

        // TODO: move grid shader and debug draw to separate debug render pass

        // Draw grid shader
        // vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //                   gridPipeline.handle());
        // vkCmdDraw(frame.cmd, 4, 1, 0, 0);

        // Debug Draw
        hk::dd::draw(frame.cmd);

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
    // bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // bar.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // bar.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    // bar.image = offscreen_.color_.image();
    // bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // bar.subresourceRange.baseMipLevel = 0;
    // bar.subresourceRange.levelCount = 1;
    // bar.subresourceRange.baseArrayLayer = 0;
    // bar.subresourceRange.layerCount = 1;
    // vkCmdPipelineBarrier(frame.cmd,
    //                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    //                      0, 0, NULL, 0, NULL, 1, &bar);
    //
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
        hk::event::EventContext context;
        context.u32[0] = window_->width();
        context.u32[1] = window_->height();
        resize(context, this);
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

    builder.setShader(hndlGridVS);
    builder.setShader(hndlGridPS);

    builder.setVertexLayout(0, 0);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          VK_CULL_MODE_NONE,
                          VK_FRONT_FACE_CLOCKWISE);
    builder.setMultisampling();

    hk::vector<VkDescriptorSetLayout> descriptorSetsLayouts = {
        global_desc_layout.handle(),
    };
    builder.setDescriptors(descriptorSetsLayouts);

    builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, offscreen_.depth_.format());
    builder.setColors({{ swapchain_.format(), hk::BlendState::DEFAULT }});

    hk::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    builder.setDynamicStates(dynamic_states);
    gridPipeline = builder.build(offscreen_.render_pass_, 1);
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
}

void Renderer::createSamplers()
{
    // TODO: make sampler wrapper with more config
    enum FilterType {
        Nearest,
        Linear,
        Anisotropic,
    };
    auto createSampler = [&](FilterType filter, VkSamplerAddressMode mode, std::string name) {
        VkResult err;
        VkSampler out;
        const auto physical_info = hk::context()->physicalInfo();

        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.mipLodBias = 0.f;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.minLod = 0.f;
        info.maxLod = 0.f;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.unnormalizedCoordinates = VK_FALSE;

        switch (filter) {
        case FilterType::Nearest: {
            info.magFilter = VK_FILTER_NEAREST;
            info.minFilter = VK_FILTER_NEAREST;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.anisotropyEnable = VK_FALSE;
            info.maxAnisotropy = 0.f;
        } break;
        case FilterType::Linear: {
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.anisotropyEnable = VK_FALSE;
            info.maxAnisotropy = 0.f;
        } break;
        case FilterType::Anisotropic: {
            info.magFilter = VK_FILTER_LINEAR;
            info.minFilter = VK_FILTER_LINEAR;
            info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            info.anisotropyEnable = VK_TRUE;
            info.maxAnisotropy = physical_info.properties.limits.maxSamplerAnisotropy;
        } break;
        }

        info.addressModeU = mode;
        info.addressModeV = mode;
        info.addressModeW = mode;

        err = vkCreateSampler(device_, &info, nullptr, &out);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Sampler");
        hk::debug::setName(out, name);
        return out;
    };

    // Nearest/Point samplers
    Samplers::Nearest &nearest = samplers_.nearest;
    nearest.repeat = createSampler(Nearest, VK_SAMPLER_ADDRESS_MODE_REPEAT,          "Global Nearest Repeat Sampler");
    nearest.mirror = createSampler(Nearest, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, "Global Nearest Mirror Sampler");
    nearest.clamp  = createSampler(Nearest, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,   "Global Nearest Clamp Sampler");
    nearest.border = createSampler(Nearest, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, "Global Nearest Border Sampler");

    // Linear samplers
    Samplers::Linear &linear = samplers_.linear;
    linear.repeat = createSampler(Linear, VK_SAMPLER_ADDRESS_MODE_REPEAT,          "Global Linear Repeat Sampler");
    linear.mirror = createSampler(Linear, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, "Global Linear Mirror Sampler");
    linear.clamp  = createSampler(Linear, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,   "Global Linear Clamp Sampler");
    linear.border = createSampler(Linear, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, "Global Linear Border Sampler");

    // Anisotropic samplers
    Samplers::Anisotopic &aniso = samplers_.anisotropic;
    aniso.repeat = createSampler(Anisotropic, VK_SAMPLER_ADDRESS_MODE_REPEAT,          "Global Anisotropic Repeat Sampler");
    aniso.mirror = createSampler(Anisotropic, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, "Global Anisotropic Mirror Sampler");
    aniso.clamp  = createSampler(Anisotropic, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,   "Global Anisotropic Clamp Sampler");
    aniso.border = createSampler(Anisotropic, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, "Global Anisotropic Border Sampler");
}

void Renderer::resize(const hk::event::EventContext &size, void *listener)
{
    Renderer *self = reinterpret_cast<Renderer*>(listener);

    vkDeviceWaitIdle(self->device_);

    self->swapchain_.recreate({size.u32[0], size.u32[1]});

    self->ui_.deinit();
    self->present_.deinit();
    self->post_process_.deinit();
    self->offscreen_.deinit();

    self->offscreen_.init(&self->swapchain_, self->global_desc_layout.handle());
    self->post_process_.init(&self->swapchain_);
    self->present_.init(&self->swapchain_);
    self->ui_.init(self->window_, &self->swapchain_);

    self->resized = false;
}
