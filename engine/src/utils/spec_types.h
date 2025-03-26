#ifndef HK_SPEC_TYPES_H
#define HK_SPEC_TYPES_H

#include "hkstl/utility/hktypes.h"

namespace hk::spec {

enum class AdapterType : u8 {
    OTHER = 0,

    INTEGRATED,
    DISCRETE,
    VIRTUAL,
    CPU,

    MAX_ADAPTER_TYPE
};

enum class AdapterVendor : u8 {
    OTHER = 0,

    AMD,
    NVIDIA,

    INTEL,

    ARM,
    QUALCOMM,

    MAX_VENDOR
};

enum class BackendType : u8 {
    OPENGL,
    VULKAN,
    DIRECTX11,
    DIRECTX12
};

enum class SystemType : u8 {
    WINDOWS,
    LINUX,
};

}

#endif // HK_SPEC_TYPES_H
