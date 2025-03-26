#ifndef HK_RESOURCE_TYPES_H
#define HK_RESOURCE_TYPES_H

#include "hkstl/utility/hktypes.h"
#include "hkstl/strings/hkstring.h"

// FIX: temp for VkImageLayout
#include "vendor/vulkan/vulkan.h"

namespace hk {

#pragma warning(disable : 4201)
template<typename Tag>
struct Handle {
    union {
        struct {
            u32 index : 20; // 1'048'575 objects should be enough for everything
            u32 gen   : 12;
        };
        u32 value = 0xdeadcell;
    };
};
#pragma warning(default : 4201)

using BufferHandle = Handle<struct BufferTag>;
using ImageHandle = Handle<struct ImageTag>;

struct ResourceMetadata {
    hk::string name;
    struct Handle<struct ResourceTag> handle;
};

/* ===== Resource Types ===== */
enum class MemoryType : u8 {
    NONE = 0,

    GPU_LOCAL,
    CPU_UPLOAD,
    CPU_READBACK,

    MAX_MEMORY_TYPE
};

enum class BufferType : u8 {
    NONE = 0,

    VERTEX_BUFFER,
    INDEX_BUFFER,
    UNIFORM_BUFFER,

    MAX_BUFFER_TYPE
};

enum class ImageType : u8 {
    NONE = 0,

    TEXTURE,

    RENDER_TARGET,
    DEPTH_BUFFER,

    MAX_IMAGE_TYPE
};

enum class Format : u16 {
    UNDEFINED = 0,

    R8_UNORM            = 0b0001'0001'0100'0100,
    R8_SNORM            = 0b0001'0001'0100'0010,
    R8_UINT             = 0b0001'0001'0001'0100,
    R8_SINT             = 0b0001'0001'0001'0010,
    R8_SRGB             = 0b0001'0001'1000'0010,
    R8G8_UNORM          = 0b0010'0001'0100'0100,
    R8G8_SNORM          = 0b0010'0001'0100'0010,
    R8G8_UINT           = 0b0010'0001'0001'0100,
    R8G8_SINT           = 0b0010'0001'0001'0010,
    R8G8_SRGB           = 0b0010'0001'1000'0010,
    R8G8B8_UNORM        = 0b0100'0001'0100'0100,
    R8G8B8_SNORM        = 0b0100'0001'0100'0010,
    R8G8B8_UINT         = 0b0100'0001'0001'0100,
    R8G8B8_SINT         = 0b0100'0001'0001'0010,
    R8G8B8_SRGB         = 0b0100'0001'1000'0010,
    R8G8B8A8_UNORM      = 0b1000'0001'0100'0100,
    R8G8B8A8_SNORM      = 0b1000'0001'0100'0010,
    R8G8B8A8_UINT       = 0b1000'0001'0001'0100,
    R8G8B8A8_SINT       = 0b1000'0001'0001'0010,
    R8G8B8A8_SRGB       = 0b1000'0001'1000'0010,

    R16_UNORM           = 0b0001'0010'0100'0100,
    R16_SNORM           = 0b0001'0010'0100'0010,
    R16_UINT            = 0b0001'0010'0001'0100,
    R16_SINT            = 0b0001'0010'0001'0010,
    R16_SFLOAT          = 0b0001'0010'0010'0010,
    R16G16_UNORM        = 0b0010'0010'0100'0100,
    R16G16_SNORM        = 0b0010'0010'0100'0010,
    R16G16_UINT         = 0b0010'0010'0001'0100,
    R16G16_SINT         = 0b0010'0010'0001'0010,
    R16G16_SFLOAT       = 0b0010'0010'0010'0010,
    R16G16B16_UNORM     = 0b0100'0010'0100'0100,
    R16G16B16_SNORM     = 0b0100'0010'0100'0010,
    R16G16B16_UINT      = 0b0100'0010'0001'0100,
    R16G16B16_SINT      = 0b0100'0010'0001'0010,
    R16G16B16_SFLOAT    = 0b0100'0010'0010'0010,
    R16G16B16A16_UNORM  = 0b1000'0010'0100'0100,
    R16G16B16A16_SNORM  = 0b1000'0010'0100'0010,
    R16G16B16A16_UINT   = 0b1000'0010'0001'0100,
    R16G16B16A16_SINT   = 0b1000'0010'0001'0010,
    R16G16B16A16_SFLOAT = 0b1000'0010'0010'0010,

    R32_UINT            = 0b0001'0100'0001'0100,
    R32_SINT            = 0b0001'0100'0001'0010,
    R32_SFLOAT          = 0b0001'0100'0010'0010,
    R32G32_UINT         = 0b0010'0100'0001'0100,
    R32G32_SINT         = 0b0010'0100'0001'0010,
    R32G32_SFLOAT       = 0b0010'0100'0010'0010,
    R32G32B32_UINT      = 0b0100'0100'0001'0100,
    R32G32B32_SINT      = 0b0100'0100'0001'0010,
    R32G32B32_SFLOAT    = 0b0100'0100'0010'0010,
    R32G32B32A32_UINT   = 0b1000'0100'0001'0100,
    R32G32B32A32_SINT   = 0b1000'0100'0001'0010,
    R32G32B32A32_SFLOAT = 0b1000'0100'0010'0010,

    R64_UINT            = 0b0001'1000'0001'0100,
    R64_SINT            = 0b0001'1000'0001'0010,
    R64_SFLOAT          = 0b0001'1000'0010'0010,
    R64G64_UINT         = 0b0010'1000'0001'0100,
    R64G64_SINT         = 0b0010'1000'0001'0010,
    R64G64_SFLOAT       = 0b0010'1000'0010'0010,
    R64G64B64_UINT      = 0b0100'1000'0001'0100,
    R64G64B64_SINT      = 0b0100'1000'0001'0010,
    R64G64B64_SFLOAT    = 0b0100'1000'0010'0010,
    R64G64B64A64_UINT   = 0b1000'1000'0001'0100,
    R64G64B64A64_SINT   = 0b1000'1000'0001'0010,
    R64G64B64A64_SFLOAT = 0b1000'1000'0010'0010,

    S8_UINT            = 0b0001'0001'0001'1100,
    D16_UNORM          = 0b0001'0010'0100'0101,
    D32_SFLOAT         = 0b0001'0100'0010'0011,
    D16_UNORM_S8_UINT  = 0b0010'0011'0101'1101,
    D24_UNORM_S8_UINT  = 0b0010'0100'0101'1101,
    D32_SFLOAT_S8_UINT = 0b0010'0101'0011'1111,
};

/* ===== Resource Descriptors ===== */
struct BufferDesc {
    BufferType type;
    MemoryType access;

    u32 size;
    u32 stride;
};

struct ImageDesc {
    hk::ImageType type;

    hk::Format format;

    u32 width;
    u32 height;
    u32 channels;

    // TODO: move out of desc
    hk::vector<VkImageLayout> layout_history;
};

}

#endif // HK_RESOURCE_TYPES_H
