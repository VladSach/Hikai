#ifndef HK_IMGUI_WRAPPER_H
#define HK_IMGUI_WRAPPER_H

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/ImGuizmo.h"

#include "vendor/vulkan/vulkan.h"

#include "platform/platform.h"

#include "math/hkmath.h"

#include <functional>

namespace hk::imgui {

HKAPI void push(const std::function<void()> &callback);

HKAPI void* addTexture(VkImageView view, VkSampler sampler);
HKAPI void removeTexture(void *texture);

void init(const Window *window, VkRenderPass pass);
void deinit();

void draw(VkCommandBuffer cmd);

namespace utils {

// TODO: wrappers
// void Combo(const char *name, const char *items[], std::function if_selected);

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
}

}

}

#endif // HK_IMGUI_WRAPPER_H
