#include "imguiwrapper.h"

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/imgui_impl_vulkan.h"

#include "vendor/imgui/ImGuizmo.h"

#ifdef HKWINDOWS
#include "vendor/vulkan/vulkan_win32.h"
#include "vendor/imgui/imgui_impl_win32.h"
#endif

#include "renderer/vkwrappers/vkcontext.h"
#include "renderer/vkwrappers/vkdebug.h"

#include <vector>

// FIX: tmp (should be set by usercode)
#include "../assets/fonts/Inter/InterVariable.h"
#include "utils/spec.h"
#include <queue>

namespace hk::imgui {

static std::vector<std::function<void()>> callbacks;
static VkDescriptorPool imgui_pool;

static std::queue<VkDescriptorSet> deletion_queue;

void push(const std::function<void()> &callback)
{
    callbacks.push_back(callback);
}

void* addTexture(ImageHandle image, VkSampler sampler)
{
    VkDescriptorSet out = ImGui_ImplVulkan_AddTexture(
        sampler, hk::bkr::view(image), hk::bkr::desc(image).layout_history.back());

    return out;
}

void removeTexture(void *texture)
{
    deletion_queue.push(reinterpret_cast<VkDescriptorSet>(texture));
}

void Image(ImageHandle image, VkSampler sampler,
           const ImVec2& image_size,
           const ImVec2& uv0, const ImVec2& uv1,
           const ImVec4& tint_col, const ImVec4& border_col)
{
    void *tmp = hk::imgui::addTexture(image, sampler);
    ImGui::Image(tmp, image_size, uv0, uv1, tint_col, border_col);
    hk::imgui::removeTexture(tmp);
}

// Vulkan Backend
void createVulkanBackend(VkRenderPass pass);
int HK_ImGui_ImplWin32_CreateVkSurface(
    ImGuiViewport* viewport,
    ImU64 vk_instance,
    const void* vk_allocator,
    ImU64* out_vk_surface);

void init(const Window *window, VkRenderPass pass)
{
    LOG_DEBUG("Creating ImGui");

    // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateVkSurface=HK_ImGui_ImplWin32_CreateVkSurface;

    // TODO: move fonts to editor
    ImFontConfig font;
    font.FontDataOwnedByAtlas = false;
    strcpy_s(font.Name, "InterVariable, 16px");

    f32 scale = hk::spec::system().monitors.at(0).scale;

    // ImFont *inter = io.Fonts->AddFontFromMemoryTTF(
    io.Fonts->AddFontFromMemoryTTF(
        hk::fonts::inter::variable::data,
        hk::fonts::inter::variable::size,
        std::round(16.f * scale), &font);

    ImGui::GetStyle().ScaleAllSizes(scale);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window->hwnd());

    createVulkanBackend(pass);
}

void deinit()
{
    LOG_DEBUG("Destroying ImGui");

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(hk::vkc::device(), imgui_pool, nullptr);
}

void draw(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    for (auto &callback : callbacks) {
        callback();
    }
    callbacks.clear();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}

void flush()
{
    while (!deletion_queue.empty()) {
        ImGui_ImplVulkan_RemoveTexture(deletion_queue.back());
        deletion_queue.pop();
    }
}


void createVulkanBackend(VkRenderPass pass)
{
    // Create descriptor pool
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

    vkCreateDescriptorPool(hk::vkc::device(), &pool_info, nullptr, &imgui_pool);
    hk::debug::setName(imgui_pool, "ImGui Descriptor Pool");

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = hk::vkc::instance();
    init_info.PhysicalDevice = hk::vkc::adapter();
    init_info.Device = hk::vkc::device();
    init_info.Queue = hk::vkc::graphics().handle();
    init_info.DescriptorPool = imgui_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.RenderPass = pass;

    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    init_info.PipelineRenderingCreateInfo = {};
    init_info.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    ImGui_ImplVulkan_Init(&init_info);

    ImGui_ImplVulkan_CreateFontsTexture();
}

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

}
