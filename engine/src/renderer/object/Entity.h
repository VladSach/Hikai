#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "defines.h"

#include "resources/AssetManager.h"

#include "Light.h"

namespace hk {

struct Entity {
    u32 id;

    hk::bitset<7> dirty;

    u32 hndlMesh = 0;
    u32 hndlMaterial = 0;
    Light *light = nullptr;

    // TODO: entity-component
    void attachMesh(u32 handle)
    {
        hndlMesh = handle;
        dirty.flip(0);
    }

    void attachMaterial(u32 handle)
    {
        hndlMaterial = handle;

        hk::MaterialAsset &asset = hk::assets()->getMaterial(handle);

        // FIX: temp, should be material callback, not shaders
        hk::assets()->attachCallback(asset.data.vertex_shader, [&](){
            dirty.flip(1);
        });
        hk::assets()->attachCallback(asset.data.pixel_shader, [&](){
            dirty.flip(1);
        });

        dirty.flip(1);
    }

    void attachLight(const Light &l)
    {
        // FIX: why would light be a pointer?
        if (!light) delete light;

        light = new Light();
        *light = l;

        dirty.flip(2);
    }
};

}

#endif // HK_ENTITY_H
