#ifndef HK_VERTEX_LAYOUT_H
#define HK_VERTEX_LAYOUT_H

#include "vendor/vulkan/vulkan.h"

#include "utils/numerics/hkbitflag.h"
#include "utils/containers/hkvector.h"

namespace hk {

enum class Format : u16 {
    NONE       = 0,

    SIGNED     = 1 << 1,
    UNSIGNED   = 1 << 2,

    INT        = 1 << 3,
    FLOAT      = 1 << 4,
    NORMALIZED = 1 << 5,
    RGB        = 1 << 6,

    B8         = 1 << 7,
    B16        = 1 << 8,
    B32        = 1 << 9,
    B64        = 1 << 10,

    VEC1       = 1 << 11,
    VEC2       = 1 << 12,
    VEC3       = 1 << 13,
    VEC4       = 1 << 14,
};

hk::vector<VkVertexInputAttributeDescription>
createVertexLayout(const hk::vector<hk::bitflag<Format>> &formats);

REGISTER_ENUM(Format);

}

#endif // HK_VERTEX_LAYOUT_H
