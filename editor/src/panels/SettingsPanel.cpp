#include "SettingsPanel.h"

#include "renderer/ui/imguiwrapper.h"
#include "renderer/vkwrappers/vkutils.h"

#include "vendor/vulkan/vk_enum_string_helper.h"

void SettingsPanel::init(Renderer *renderer)
{
    renderer_ = renderer;

    is_open_ = false;
    enable_post_process_ = true;

    hk::event::subscribe(hk::event::EVENT_WINDOW_RESIZE,
        [&](const hk::event::EventContext &context, void *listener) {
            (void)context; (void)listener;

            // if (viewport_image_) {
            //     hk::imgui::removeTexture(viewport_image_);
            // }

            viewport_image_ = hk::imgui::addTexture(renderer_->post_process_.color_.view(),
                                                    renderer_->samplerLinear);
    }, this);
}

void SettingsPanel::deinit()
{
}

void SettingsPanel::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Settings", &is_open_)) {

            if (ImGui::CollapsingHeader("Vulkan")) {
                addInstanceSettings();
                addSwapchainSettings();
            }

            if (ImGui::CollapsingHeader("Shaders", ImGuiTreeNodeFlags_DefaultOpen)) {
                addShaderSettings();
            }


        } ImGui::End();
    });
}

void SettingsPanel::addInstanceSettings()
{
    if (ImGui::TreeNode("Instance")) {
        hk::VulkanContext::InstanceInfo &info = hk::context()->instanceInfo();
        ImGui::Text("API Version: %s",
                    hk::vkApiToString(info.apiVersion).c_str());

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

        ImGui::TreePop();
    }
}

void SettingsPanel::addSwapchainSettings()
{
    hk::Swapchain &swapchain = renderer_->swapchain_;

    if (ImGui::TreeNode("Swapchain")) {
        if (ImGui::TreeNode("Surface Formats")) {
            for (u32 i = 0; i < swapchain.info().formats.size(); ++i) {
                auto &format = swapchain.info().formats.at(i);

                ImGui::PushID(i);

                // b8 isFormat = false;
                // if (swapchain.surfaceFormat.format == format.format &&
                //     swapchain.surfaceFormat.colorSpace == format.colorSpace)
                // {
                //     isFormat = true;
                // }
                //
                // if (ImGui::Checkbox("", &isFormat)) {
                //     swapchain.setSurfaceFormat(format);
                //     renderer->resized = true;
                // }
                //
                // ImGui::SameLine();

                if (ImGui::TreeNode(string_VkFormat(format.format))) {
                    ImGui::Text("Colorspace: %s",
                        string_VkColorSpaceKHR(format.colorSpace));

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Present Modes")) {
            for (u32 i = 0; i < swapchain.info().present_modes.size(); ++i) {
                auto &mode = swapchain.info().present_modes.at(i);

                ImGui::PushID(i);

                // b8 isMode = false;
                // if (swapchain.presentMode == mode) { isMode = true; }
                //
                // if (ImGui::Checkbox("", &isMode)) {
                //     swapchain.setPresentMode(mode);
                //     renderer->resized = true;
                // }
                //
                // ImGui::SameLine();

                ImGui::Text("%s", string_VkPresentModeKHR(mode));

                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Surface Capabilities")) {
            const auto &caps = swapchain.info().caps;
            ImGui::Text("minImageCount: %d", caps.minImageCount);
            ImGui::Text("maxImageCount: %d", caps.maxImageCount);

            ImGui::Spacing();

            ImGui::Text("currentExtent:  %d x %d",
                        caps.currentExtent.width, caps.currentExtent.height);
            ImGui::Text("minImageExtent: %d x %d",
                        caps.minImageExtent.width, caps.minImageExtent.height);
            ImGui::Text("maxImageExtent: %d x %d",
                        caps.maxImageExtent.width, caps.maxImageExtent.height);

            ImGui::Spacing();

            ImGui::Text("maxImageArrayLayers: %d", caps.maxImageArrayLayers);

            ImGui::Spacing();

            // TODO: supportedTransforms and currentTransform;

            ImGui::Spacing();

            if (ImGui::TreeNode("supportedCompositeAlpha")) {
                ImGuiTableFlags flags = ImGuiTableFlags_RowBg |
                                        ImGuiTableFlags_Resizable |
                                        ImGuiTableFlags_BordersOuter;
                if (ImGui::BeginTable("Supported Composite Alpha", 2, flags)) {
                    // ImGui::TableSetupColumn("Bit");
                    // ImGui::TableSetupColumn("Supported");
                    // ImGui::TableHeadersRow();

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR");
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ? 'X' : ' ');

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR");
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ? 'X' : ' ');

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR");
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ? 'X' : ' ');

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR");
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? 'X' : ' ');

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }

            ImGui::Spacing();

            // TODO: supportedUsageFlags

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

void SettingsPanel::addShaderSettings()
{
    if (ImGui::TreeNode("Deferred Rendering")) {

        constexpr const char *items[] = {
            "Final Color",
            "Position",
            "Normal",
            "Albedo",
            "Material",
            "Depth",
        };

        static u32 curr = 0;
        const char *preview = items[curr];
        if (ImGui::BeginCombo("Buffer", preview)) {
            for (u32 i = 0; i < 6; ++i) {
                const b8 selected = (curr == i);

                if (ImGui::Selectable(items[i], selected)) {
                    curr = i;

                    // FIX: temp
                    if (i == 0) viewport_image_ = hk::imgui::addTexture(renderer_->post_process_.color_.view(), renderer_->samplerLinear);
                    else if (i == 1) viewport_image_ = hk::imgui::addTexture(renderer_->offscreen_.position_.view(), renderer_->samplerLinear);
                    else if (i == 2) viewport_image_ = hk::imgui::addTexture(renderer_->offscreen_.normal_.view(), renderer_->samplerLinear);
                    else if (i == 3) viewport_image_ = hk::imgui::addTexture(renderer_->offscreen_.albedo_.view(), renderer_->samplerLinear);
                    else if (i == 4) viewport_image_ = hk::imgui::addTexture(renderer_->offscreen_.material_.view(), renderer_->samplerLinear);
                    else if (i == 5) viewport_image_ = hk::imgui::addTexture(renderer_->offscreen_.depth_.view(), renderer_->samplerLinear);
                }

                if (selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("PBR")) {

        // TODO: choose between BRDFs, NDFs, and others

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Post Process", ImGuiTreeNodeFlags_DefaultOpen)) {
        b8 changed = false;
        changed = ImGui::Checkbox("Enabled", &enable_post_process_);

        if (changed) {
            // hk::event::EventContext context;
            // context.u32[0] = renderer_->window_->width();
            // context.u32[1] = renderer_->window_->height();
            // hk::event::fire(hk::event::EVENT_WINDOW_RESIZE, context);
            viewport_image_ = enable_post_process_ ?
                hk::imgui::addTexture(renderer_->post_process_.color_.view(), renderer_->samplerLinear) :
                hk::imgui::addTexture(renderer_->offscreen_.color_.view(), renderer_->samplerLinear);
        }

        // ImGui::InputFloat("Exposure", ev100);

        // Tone Mapping selection
        constexpr const char *items[] = {
            "None",
            "Reinhard",
            "ACES",
            "Custom",
        };

        static u32 curr = 0;
        const char *preview = items[curr];
        if (ImGui::BeginCombo("Tone Mapping", preview)) {
            for (u32 i = 0; i < 4; ++i) {
                const b8 selected = (curr == i);

                if (ImGui::Selectable(items[i], selected)) {
                    curr = i;
                    // TODO: change tone mapping
                }

                if (selected) { ImGui::SetItemDefaultFocus(); }
            }
            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }

    // if (ImGui::CollapsingHeader("Shaders")) {
    //     constexpr const char *items[] = {
    //         "Default",
    //         "Normals",
    //         "Texture",
    //         "Depth",
    //         "Grid"
    //     };
    //     u32 handles[] = {
    //         hndlDefaultPS,
    //         hndlNormalsPS,
    //         hndlTexturePS,
    //     };
    //     static u32 curr = 0;
    //
    //     const char *preview = items[curr];
    //     if (ImGui::BeginCombo("Shader groups", preview)) {
    //         for (u32 i = 0; i < 4; ++i) {
    //             const b8 selected = (curr == i);
    //
    //             if (ImGui::Selectable(items[i], selected)) {
    //                 curr = i;
    //                 curShaderPS = handles[curr];
    //                 resized = true;
    //             }
    //
    //             if (selected) { ImGui::SetItemDefaultFocus(); }
    //         }
    //         ImGui::EndCombo();
    //     }
    //
    //     ImGui::Separator();
    //
    //     ImGui::Text("Grid Shader Values");
    //
    //     // static f32 grid_size = 100.0f;
    //     // ImGui::InputFloat("Grid Size", &grid_size, 5);
    // }
}
