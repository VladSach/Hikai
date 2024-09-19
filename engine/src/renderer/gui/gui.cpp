#include "gui.h"

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_win32.h"
#include "vendor/imgui/imgui_impl_vulkan.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

#include "renderer/VulkanContext.h"

#include "GuiLog.h"

#include "platform/Monitor.h"

int HK_ImGui_ImplWin32_CreateVkSurface(
    ImGuiViewport* viewport,
    ImU64 vk_instance,
    const void* vk_allocator,
    ImU64* out_vk_surface)
{
    VkInstance instance = (VkInstance)vk_instance;
    VkAllocationCallbacks* allocator = (VkAllocationCallbacks*)vk_allocator;

    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)viewport->PlatformHandle;
    createInfo.hinstance = GetModuleHandle(NULL);

    VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, allocator,
                                              (VkSurfaceKHR*)out_vk_surface);
    return result;
}

void GUI::init(const Window *window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // WARN: Introduces annoying flickering, so turned off for now
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    // ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    // platform_io.Platform_CreateVkSurface=HK_ImGui_ImplWin32_CreateVkSurface;

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(reinterpret_cast<const WinWindow*>(window)->getHWnd());
    createVulkanBackend();

    addImGuiLog();
}

void GUI::deinit()
{
    vkDestroyRenderPass(hk::context()->device(), uiRenderPass, nullptr);
    uiRenderPass = nullptr;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();

    removeImGuiLog();
}

void GUI::setViewportMode(VkImageView view)
{
    viewportMode = true;

    createVulkanBackend();

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

    hk::VulkanContext &context = *hk::context();
    vkCreateSampler(context.device(), &samplerInfo, nullptr, &viewportSampler_);

    viewportImage = ImGui_ImplVulkan_AddTexture(
        viewportSampler_,
        view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GUI::setOverlayMode()
{
    viewportMode = false;
    created = false;
    createVulkanBackend();
}

void GUI::draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiID mainDockSpaceID;
    if (viewportMode) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Options")) {

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
        mainDockSpaceID = ImGui::DockSpaceOverViewport();
    }


    if (viewportMode & !created) {
        created = true;

        ImGui::DockBuilderAddNode(mainDockSpaceID);

        upper = mainDockSpaceID;
        left  = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Left, 0.25f, nullptr, &upper);
        lower = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Down, 0.25f, nullptr, &upper);
        right = ImGui::DockBuilderSplitNode(upper, ImGuiDir_Right, 0.25f, nullptr, &upper);

        ImGui::DockBuilderDockWindow("Viewport", upper);
        ImGui::DockBuilderDockWindow("Log", lower);
        ImGui::DockBuilderDockWindow("Scene", left);
        ImGui::DockBuilderDockWindow("Inspector", right);
    }

    if (viewportMode) {
        ImGui::Begin("Viewport");

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ImGui::Image(viewportImage,
                     ImVec2{viewportPanelSize.x, viewportPanelSize.y});

        // lockedInput = !ImGui::IsWindowFocused();
        lockedInput = !ImGui::IsWindowHovered();
        ImGui::End();
    }

    drawLog();

    if (viewportMode) { ImGui::SetNextWindowDockID(left); }
    if (ImGui::Begin("Monitors Info")) {
        hk::vector<hk::platform::MonitorInfo> infos = hk::platform::getMonitorInfos();

        ImGui::Text("Monitors found: %i", infos.size());
        for (auto &info : infos) {
            ImGui::Separator();

            ImGui::Text("Name: %s", info.name.c_str());
            ImGui::Text("Resolution: %i x %i", info.width, info.height);
            ImGui::Text("DPI: %1f", info.dpi);
            ImGui::Text("Refresh rate: %i hz", info.hz);
            ImGui::Text("Color depth: %i", info.depth);
        }
    } ImGui::End();


    if (viewportMode) { ImGui::SetNextWindowDockID(left); }
    if (ImGui::Begin("Vulkan")) {
        if (ImGui::CollapsingHeader("Instance")) {
            hk::VulkanContext::InstanceInfo &info = hk::context()->instanceInfo();
            ImGui::Text("API Version: %s", hk::vkApiToString(info.apiVersion).c_str());

            ImGui::Separator();

            if (ImGui::TreeNode("Extensions")) {
                for (u32 i = 0; i < info.extensions.size(); ++i) {
                    auto &ext = info.extensions.at(i);

                    ImGui::PushID(i);
                    if (ImGui::Checkbox("", &ext.first)) {
                        // hk::context()->deinit();
                        // hk::context()->init();
                    }
                    ImGui::SameLine();

                    if (ImGui::TreeNode(ext.second.extensionName)) {
                        ImGui::Text("Ext Version: %d", ext.second.specVersion);

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Layers")) {
                for (u32 i = 0; i < info.layers.size(); ++i) {
                    auto &layer = info.layers.at(i);

                    ImGui::PushID(i);
                    ImGui::Checkbox("", &layer.first);
                    ImGui::SameLine();

                    if (ImGui::TreeNode(layer.second.layerName)) {
                        ImGui::Text("Desc: %s", layer.second.description);
                        ImGui::Text("Spec Version: %s",
                                    hk::vkApiToString(layer.second.specVersion).c_str());
                        ImGui::Text("Implementation Version: %d",
                                    layer.second.implementationVersion);

                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        if (ImGui::CollapsingHeader("Physical Device")) {
            const auto &infos = hk::context()->physicalInfos();

            for (u32 i = 0; i < infos.size(); ++i) {
                auto &device = infos.at(i);
                auto &properties = device.properties;
                if (ImGui::TreeNode(properties.deviceName)) {
                    ImGui::Text("Type: %s", hk::vkDeviceTypeToString(properties.deviceType).c_str());
                    ImGui::Text("API Version: %s", hk::vkApiToString(properties.apiVersion).c_str());
                    ImGui::Text("Driver Version: %s", hk::vkDeviceDriveToString(properties.driverVersion, properties.vendorID).c_str());
                    ImGui::Text("Vendor: %s", hk::vkDeviceVendorToString(properties.vendorID).c_str());
                    // ImGui::Text("Device ID: %d", device.deviceID);

                    if (ImGui::TreeNode("Queue Families")) {
                        ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
                        flags |= ImGuiTableFlags_BordersOuter;
                        if (ImGui::BeginTable("Families", 6, flags)) {
                            ImGui::TableSetupColumn("Index");
                            ImGui::TableSetupColumn("Graphics");
                            ImGui::TableSetupColumn("Compute");
                            ImGui::TableSetupColumn("Transfer");
                            ImGui::TableSetupColumn("Sparse");
                            ImGui::TableSetupColumn("Protected");
                            ImGui::TableHeadersRow();
                            for (u32 j = 0; j < infos.at(i).families.size(); ++j) {
                                auto &family = infos.at(i).families.at(j);
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%u", j);
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%c", (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%c", (family.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                                ImGui::TableSetColumnIndex(3);
                                ImGui::Text("%c", (family.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                                ImGui::TableSetColumnIndex(4);
                                ImGui::Text("%c", (family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                                ImGui::TableSetColumnIndex(5);
                                ImGui::Text("%c", (family.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');
                            }
                            ImGui::EndTable();
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Extensions")) {
                        for (auto &ext : device.extensions) {
                            if (ImGui::TreeNode(ext.extensionName)) {
                                ImGui::Text("Spec Version: %d", ext.specVersion);

                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

            }
        }

        if (ImGui::CollapsingHeader("Logical Device")) {
            hk::VulkanContext::LogicalDeviceInfo info = hk::context()->deviceInfo();

            if (ImGui::TreeNode("Chosen Queues")) {
                ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
                flags |= ImGuiTableFlags_BordersOuter;

                ImGui::Text("Graphics Family");

                if (ImGui::BeginTable("Graphics Family", 6, flags)) {
                    ImGui::TableSetupColumn("Index");
                    ImGui::TableSetupColumn("Graphics");
                    ImGui::TableSetupColumn("Compute");
                    ImGui::TableSetupColumn("Transfer");
                    ImGui::TableSetupColumn("Sparse");
                    ImGui::TableSetupColumn("Protected");
                    ImGui::TableHeadersRow();

                    hk::QueueFamily &family = info.graphicsFamily;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%u", family.index_);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                    ImGui::EndTable();
                }

                ImGui::Text("Compute Family");

                if (ImGui::BeginTable("Compute Family", 6, flags)) {
                    ImGui::TableSetupColumn("Index");
                    ImGui::TableSetupColumn("Graphics");
                    ImGui::TableSetupColumn("Compute");
                    ImGui::TableSetupColumn("Transfer");
                    ImGui::TableSetupColumn("Sparse");
                    ImGui::TableSetupColumn("Protected");
                    ImGui::TableHeadersRow();

                    hk::QueueFamily &family = info.computeFamily;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%u", family.index_);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                    ImGui::EndTable();
                }

                ImGui::Text("Transfer Family");

                if (ImGui::BeginTable("Transfer Family", 6, flags)) {
                    ImGui::TableSetupColumn("Index");
                    ImGui::TableSetupColumn("Graphics");
                    ImGui::TableSetupColumn("Compute");
                    ImGui::TableSetupColumn("Transfer");
                    ImGui::TableSetupColumn("Sparse");
                    ImGui::TableSetupColumn("Protected");
                    ImGui::TableHeadersRow();

                    hk::QueueFamily &family = info.transferFamily;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%u", family.index_);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_COMPUTE_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_TRANSFER_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? 'X' : ' ');
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%c", (family.properties.queueFlags & VK_QUEUE_PROTECTED_BIT) ? 'X' : ' ');

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }
        }

    } ImGui::End();

    // ImGui::ShowDemoWindow();

    for (auto &callback : callbacks) {
        callback();
    }
    callbacks.clear();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    // ImGui::UpdatePlatformWindows();
    // ImGui::RenderPlatformWindowsDefault();
}

b8 GUI::isInputLocked() const
{
    return lockedInput;
}

void GUI::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = viewportMode ?
        VK_ATTACHMENT_LOAD_OP_CLEAR :
        VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = viewportMode ?
        VK_IMAGE_LAYOUT_UNDEFINED :
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkAttachmentDescription attachments[] = {
        colorAttachment,
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
    renderPassInfo.attachmentCount =
        sizeof(attachments)/ sizeof(attachments[0]);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    err = vkCreateRenderPass(hk::context()->device(), &renderPassInfo,
                             nullptr, &uiRenderPass);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Render Pass");
}

void GUI::createVulkanBackend()
{
    hk::VulkanContext &context = *hk::context();

    // create descriptor pool for IMGUI
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = sizeof(pool_sizes)/sizeof(pool_sizes[0]);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imguiPool;
    vkCreateDescriptorPool(context.device(), &pool_info, nullptr, &imguiPool);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = context.instance();
    init_info.PhysicalDevice = context.physical();
    init_info.Device = context.device();
    init_info.Queue = context.graphics().handle();
    init_info.DescriptorPool = imguiPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (uiRenderPass) {
        vkDestroyRenderPass(hk::context()->device(), uiRenderPass, nullptr);
        uiRenderPass = nullptr;
        ImGui_ImplVulkan_Shutdown();
    }
    createRenderPass();
    init_info.RenderPass = uiRenderPass;

    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    init_info.PipelineRenderingCreateInfo = {};
    init_info.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();

    // ImGui_ImplVulkan_DestroyFontsTexture();
}
