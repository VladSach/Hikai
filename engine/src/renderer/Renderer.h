#ifndef HK_RENDERER_H
#define HK_RENDERER_H

#include "defines.h"
#include "core/EventSystem.h"
#include "platform/platform.h"

#include "BackendVulkan.h"

enum class RenderBackend {
    NONE,
    VULKAN,
    DIRECTX12,
    DIRECTX11,
};

class Renderer {
public:
    void init(RenderBackend api, const Window &window);
    void deinit();

    void render();

    static void onResize(hk::EventContext size);

private:
    bool createRenderBackend(RenderBackend api, const Window &window);

private:
    Backend *backend;
};

#endif // HK_RENDERER_H
