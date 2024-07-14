#ifndef HK_VERTEX_H
#define HK_VERTEX_H

#include "math/hkmath.h"

struct vertex {
    hkm::vec2f pos;
    hkm::vec2f tc; // texture coordinates
    hkm::vec3f normal;
    hkm::vec3f tangent;
    hkm::vec3f bitangent;

    vertex() :
        pos(0),
        tc(0),
        normal(0),
        tangent(0),
        bitangent(0)
    {};
};

#endif // HK_VERTEX_H
