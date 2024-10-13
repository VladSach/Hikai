#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "defines.h"

namespace hk {

struct Entity {
    u32 id;
    u32 hndlMesh = 0;
    u32 hndlMaterial = 0;

    // TODO: entity-component
    void attachMesh(u32 handle)
    {
        hndlMesh = handle;
    }

    void attachMaterial(u32 handle)
    {
        hndlMaterial = handle;
    }
};

}

#endif // HK_ENTITY_H
