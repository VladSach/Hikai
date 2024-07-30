#ifndef HK_GUI_H
#define HK_GUI_H

#include "vendor/vulkan/vulkan.h"

#include "platform/platform.h"
#include "utils/containers/hkvector.h"

#include <functional>

class GUI {
public:
    void init(const Window *window);
    void deinit();

    void draw(VkCommandBuffer cmd);

    inline void pushCallback(const std::function<void()> &callback)
    {
        callbacks.push_back(callback);
    }

    // inline bool captureInput() const
    // {
    //     auto& io = ImGui::GetIO();
    //     if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
    //         return true;
    //     }
    //
    //     return false;
    // }

    // FIX: temp
    VkRenderPass uiRenderPass;

private:
    void setStyle();

    void createRenderPass();

private:
    hk::vector<std::function<void()>> callbacks;
};

#endif // HK_GUI_H
