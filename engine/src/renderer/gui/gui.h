#ifndef HK_GUI_H
#define HK_GUI_H

#include "vendor/vulkan/vulkan.h"

#include "platform/platform.h"
#include "utils/containers/hkvector.h"

#include <functional>
#include <vector>

// INFO: For Dock Builder
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/ImGuizmo.h"

#include "renderer/vkwrappers/Image.h"

// TODO: move to gui utils
#include "math/hkmath.h"
inline void drawMatrix4x4(const hkm::mat4f &matrix)
{
    ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
    flags |= ImGuiTableFlags_BordersOuter;
    if (ImGui::BeginTable("Matrix", 4, flags)) {
        for (u32 i = 0; i < 4; ++i) {
            ImGui::TableNextRow();
            for (u32 j = 0; j < 4; ++j) {
                ImGui::TableSetColumnIndex(j);
                ImGui::Text("%f", matrix(i, j));
            }
        }
        ImGui::EndTable();
    }
};

class GUI {
public:
    void init(const Window *window);
    void deinit();

    void setViewportMode(VkImageView view);
    void setOverlayMode();

    void draw(VkCommandBuffer cmd);

    HKAPI void* addTexture(hk::Image *texture);

    // TODO: rename to something like "isMouseInsideViewport"
    HKAPI b8 isInputLocked() const;
    HKAPI inline void pushCallback(const std::function<void()> &callback)
    {
        callbacks.push_back(callback);
    }

    // FIX: temp
    VkRenderPass uiRenderPass;

private:
    void setStyle();

    void createRenderPass();
    void createVulkanBackend();

private:
    std::vector<std::function<void()>> callbacks;

    b8 lockedInput = false;

    b8 viewportMode = false;
    b8 created = false;

    b8 showImGuiDemo = false;

    ImGuiID upper;
    ImGuiID lower;
    ImGuiID left;
    ImGuiID right;
    VkSampler viewportSampler_;
    VkDescriptorSet viewportImage;
};

#endif // HK_GUI_H
