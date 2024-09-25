#ifndef HK_GUI_H
#define HK_GUI_H

#include "vendor/vulkan/vulkan.h"

#include "platform/platform.h"
#include "utils/containers/hkvector.h"

#include <functional>

// INFO: For Dock Builder
#include "vendor/imgui/imgui_internal.h"

class GUI {
public:
    void init(const Window *window);
    void deinit();

    void setViewportMode(VkImageView view);
    void setOverlayMode();

    void draw(VkCommandBuffer cmd);

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
    hk::vector<std::function<void()>> callbacks;

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
