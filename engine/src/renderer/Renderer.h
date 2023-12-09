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

    void setUniformBuffer(const hkm::vec2f &res, f32 time)
    {
        backend->setUniformBuffer(res, time);
    }

    static void onResize(hk::EventContext size);

private:
    bool createRenderBackend(RenderBackend api, const Window &window);

private:
    Backend *backend;
};

#endif // HK_RENDERER_H
