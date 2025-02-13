#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "defines.h"

#include "resources/AssetManager.h"

#include "Light.h"

namespace hk {

struct Entity {
    u32 id;
    u32 hndlMesh = 0;
    u32 hndlMaterial = 0;
    Light *light = nullptr;

    u8 dirty = 0; // TODO: change to bitfield

    // TODO: entity-component
    void attachMesh(u32 handle)
    {
        hndlMesh = handle;
        dirty ^= 0b10000000;
    }

    void attachMaterial(u32 handle)
    {
        hndlMaterial = handle;

        hk::MaterialAsset &asset = hk::assets()->getMaterial(handle);

        // FIX: temp, should be material callback, not shaders
        hk::assets()->attachCallback(asset.data.hndlVS, [&](){
            dirty ^= dirty ? 0 : 0b01000000;
        });
        hk::assets()->attachCallback(asset.data.hndlPS, [&](){
            dirty ^= dirty ? 0 : 0b01000000;
        });

        dirty ^= 0b01000000;
    }

    void attachLight(const Light &l)
    {
        if (!light) { return; }

        light = new Light();
        light->color = l.color;
        light->intensity = l.intensity;

        dirty ^= 0b00100000;
    }
};

}

#endif // HK_ENTITY_H
