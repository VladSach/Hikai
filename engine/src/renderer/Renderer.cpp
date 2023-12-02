#include "Renderer.h"

void Renderer::init(RenderBackend api)
{
    if (!createRenderBackend(api)) {
        LOG_FATAL("Failed to create renderer");
        return;
    }

    backend->init();

    EventSystem::instance()->subscribe(hk::EVENT_WINDOW_RESIZE, onResize);
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
    return;
}
