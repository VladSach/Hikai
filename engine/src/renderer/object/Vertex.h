#ifndef HK_VERTEX_H
#define HK_VERTEX_H

#include "hkstl/math/hkmath.h"

struct Vertex {
    hkm::vec3f pos;
    hkm::vec3f normal;
    hkm::vec2f tc;
    hkm::vec3f tangent;
    hkm::vec3f bitangent;
    u32 pad[2];

    constexpr b8 operator ==(const Vertex &other) const {
        return (
            this->pos    == other.pos &&
            this->normal == other.normal &&
            this->tc     == other.tc
        );
    }
};

#endif // HK_VERTEX_H
