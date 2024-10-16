#ifndef HK_ENTITY_H
#define HK_ENTITY_H

#include "defines.h"

namespace hk {

struct Entity {
    u32 id;
    u32 hndlMesh = 0;
    u32 hndlMaterial = 0;

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
        dirty ^= 0b01000000;
    }
};

}

#endif // HK_ENTITY_H
