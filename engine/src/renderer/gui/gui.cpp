#include "gui.h"

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_win32.h"
#include "vendor/imgui/imgui_impl_vulkan.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#endif

#include "renderer/VulkanContext.h"

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

    VkResult result =
        vkCreateWin32SurfaceKHR(instance, &createInfo,
                                allocator, (VkSurfaceKHR*)out_vk_surface);
    return result;
}

void GUI::init(const Window *window)
{
    hk::VulkanContext &context = *hk::context();

    // create descriptor pool for IMGUI
    // the size of the pool is very oversize,
    // but it's copied from imgui demo itself.
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

void GUI::draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    // VkSampler sampler;
    // VkDevice device = hk::context()->device();
    //
    // VkSamplerCreateInfo samplerInfo = {};
    // samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // samplerInfo.magFilter = VK_FILTER_LINEAR;
    // samplerInfo.minFilter = VK_FILTER_LINEAR;
    // samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    //
    // VkPhysicalDeviceProperties properties = {};
    // vkGetPhysicalDeviceProperties(hk::context()->physical(), &properties);
    //
    // samplerInfo.anisotropyEnable = VK_TRUE;
    // samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    // samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // samplerInfo.compareEnable = VK_FALSE;
    // samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // samplerInfo.mipLodBias = 0.0f;
    // samplerInfo.minLod = 0.0f;
    // samplerInfo.maxLod = 0.0f;
    //
    // vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
    //
    // VkDescriptorSet m_Dset = ImGui_ImplVulkan_AddTexture(
    //     sampler,
    //     image,
    //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    //
    // ImGui::Begin("Viewport");
    //
    // ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    // ImGui::Image(m_Dset, ImVec2{viewportPanelSize.x, viewportPanelSize.y});
    //
    // ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void GUI::createRenderPass()
{
    VkResult err;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
