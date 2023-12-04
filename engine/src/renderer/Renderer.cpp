#include "Renderer.h"

void Renderer::init(RenderBackend api, const Window &window)
{
    if (!createRenderBackend(api)) {
        EventSystem::instance()->fireEvent(hk::EVENT_APP_SHUTDOWN,
                                    { hk::ERROR_UNSUPPORTED_GRAPHICS_API });
        return;
    }

    backend->init(window);

    EventSystem::instance()->subscribe(hk::EVENT_WINDOW_RESIZE, onResize);
}

void Renderer::deinit()
{
    backend->deinit();
}

void Renderer::render()
{
    backend->draw();
}

bool Renderer::createRenderBackend(RenderBackend api)
{
    switch (api) {
    case RenderBackend::VULKAN:
    {
        backend = new BackendVulkan();
        return true;
    } break;
    case RenderBackend::DIRECTX12:
    case RenderBackend::DIRECTX11:
    default:
        return false;
    }
}

void Renderer::onResize(hk::EventContext data)
{
    // TODO: add resize event for backends
    return;
}
