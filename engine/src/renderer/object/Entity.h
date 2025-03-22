#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "resources/AssetManager.h"
#include "core/events.h"

#include "hkstl/numerics/hkbitset.h"

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

        // FIX: material event should be subscribed by handle
        // otherwise entity receives events from all materials
        hk::event::subscribe(hk::event::EVENT_MATERIAL_MODIFIED,
            [&](const hk::event::EventContext &context, void *listener) {
                (void)listener;

                u32 handle = context.u32[0];
                if (handle == hndlMaterial) { dirty.flip(1); }
            },
        this);

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
