#include "thumbnails.h"

#include "renderer/ui/imguiwrapper.h"

namespace hke::thumbnail {

static Renderer *r;

// void* == VkDescriptorSet
static std::unordered_map<u32, void*> cache;

void init(Renderer *renderer)
{
    r = renderer;
    cache.clear();
}

void* get(u32 handle)
{
    if (cache.count(handle)) { return cache[handle]; }

    hk::Image *image = hk::assets()->getTexture(handle).texture;

    cache[handle] = hk::imgui::addTexture(image->view(), r->samplerLinear);

    return cache[handle];
}

void remove(u32 handle)
{
    cache.erase(handle);
}

}
