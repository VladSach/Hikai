#ifndef HK_VERTEX_H
#define HK_VERTEX_H

#include "math/hkmath.h"

struct Vertex {
    hkm::vec3f pos;
    hkm::vec3f normal;
    hkm::vec2f tc;
    // hkm::vec3f color;

    constexpr b8 operator ==(const Vertex &other) const {
        return (
            this->pos    == other.pos &&
            this->normal == other.normal &&
            this->tc     == other.tc
        );
    }
};

#endif // HK_VERTEX_H
