#include "thumbnails.h"

namespace hke::thumbnail {

static GUI *g;

// void* == VkDescriptorSet
static std::unordered_map<u32, void*> cache;

void init(GUI *gui)
{
    g = gui;
    cache.clear();
}

void* get(u32 handle)
{
    if (cache.count(handle)) { return cache[handle]; }

    hk::Image *image = hk::assets()->getTexture(handle).texture;

    cache[handle] = g->addTexture(image);
    return cache[handle];
}

void remove(u32 handle)
{
    cache.erase(handle);
}

}
