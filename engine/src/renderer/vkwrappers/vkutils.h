#ifndef HK_VKUTILS_H
#define HK_VKUTILS_H

#include "vendor/vulkan/vulkan.h"
#include "renderer/resources.h"

namespace hk {

/* ===== Vulkan Resources ===== */
constexpr VkMemoryPropertyFlags to_vulkan(MemoryType type)
{
    VkMemoryPropertyFlags out = 0;

    switch (type) {
    case MemoryType::GPU_LOCAL: {
        out = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    } break;

    case MemoryType::CPU_UPLOAD: {
        out = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } break;

    case MemoryType::CPU_READBACK: {
        out = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
              VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } break;

    default: { break; }
    }

    return out;
}

constexpr VkBufferUsageFlags to_vulkan(BufferType type)
{
    VkBufferUsageFlags out = 0;

    switch (type) {
    case BufferType::VERTEX_BUFFER: {
        out = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    } break;

    case BufferType::INDEX_BUFFER: {
        out = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    } break;

    case BufferType::UNIFORM_BUFFER: {
        out = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    } break;

    default: { break; }
    }

    return out;
}

constexpr VkImageUsageFlags to_vulkan(ImageType type)
{
    VkImageUsageFlags out = 0;

    switch (type) {

    case ImageType::TEXTURE: {
        out = VK_IMAGE_USAGE_SAMPLED_BIT;
    } break;

    case ImageType::RENDER_TARGET: {
        out = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // FIX: temp, until deferred uses subpasses
        out |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    } break;

    case ImageType::DEPTH_BUFFER: {
        out = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        // FIX: temp, until deferred uses subpasses
        out |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    } break;

    default: { break; }
    }

    // FIX: temp, because imgui uses combined image sampler
    out |= VK_IMAGE_USAGE_SAMPLED_BIT;

    return out;
}

constexpr VkFormat to_vulkan(Format format)
{
    constexpr std::pair<Format, VkFormat> lookup_map[] =
    {
        { Format::UNDEFINED,      VK_FORMAT_UNDEFINED },

        { Format::R8_UNORM,       VK_FORMAT_R8_UNORM       },
        { Format::R8_SNORM,       VK_FORMAT_R8_SNORM       },
        { Format::R8_UINT,        VK_FORMAT_R8_UINT        },
        { Format::R8_SINT,        VK_FORMAT_R8_SINT        },
        { Format::R8_SRGB,        VK_FORMAT_R8_SRGB        },
        { Format::R8G8_UNORM,     VK_FORMAT_R8G8_UNORM     },
        { Format::R8G8_SNORM,     VK_FORMAT_R8G8_SNORM     },
        { Format::R8G8_UINT,      VK_FORMAT_R8G8_UINT      },
        { Format::R8G8_SINT,      VK_FORMAT_R8G8_SINT      },
        { Format::R8G8_SRGB,      VK_FORMAT_R8G8_SRGB      },
        { Format::R8G8B8_UNORM,   VK_FORMAT_R8G8B8_UNORM   },
        { Format::R8G8B8_SNORM,   VK_FORMAT_R8G8B8_SNORM   },
        { Format::R8G8B8_UINT,    VK_FORMAT_R8G8B8_UINT    },
        { Format::R8G8B8_SINT,    VK_FORMAT_R8G8B8_SINT    },
        { Format::R8G8B8_SRGB,    VK_FORMAT_R8G8B8_SRGB    },
        { Format::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
        { Format::R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM },
        { Format::R8G8B8A8_UINT,  VK_FORMAT_R8G8B8A8_UINT  },
        { Format::R8G8B8A8_SINT,  VK_FORMAT_R8G8B8A8_SINT  },
        { Format::R8G8B8A8_SRGB,  VK_FORMAT_R8G8B8A8_SRGB  },

        { Format::R16_UNORM,           VK_FORMAT_R16_UNORM           },
        { Format::R16_SNORM,           VK_FORMAT_R16_SNORM           },
        { Format::R16_UINT,            VK_FORMAT_R16_UINT            },
        { Format::R16_SINT,            VK_FORMAT_R16_SINT            },
        { Format::R16_SFLOAT,          VK_FORMAT_R16_SFLOAT          },
        { Format::R16G16_UNORM,        VK_FORMAT_R16G16_UNORM        },
        { Format::R16G16_SNORM,        VK_FORMAT_R16G16_SNORM        },
        { Format::R16G16_UINT,         VK_FORMAT_R16G16_UINT         },
        { Format::R16G16_SINT,         VK_FORMAT_R16G16_SINT         },
        { Format::R16G16_SFLOAT,       VK_FORMAT_R16G16_SFLOAT       },
        { Format::R16G16B16_UNORM,     VK_FORMAT_R16G16B16_UNORM     },
        { Format::R16G16B16_SNORM,     VK_FORMAT_R16G16B16_SNORM     },
        { Format::R16G16B16_UINT,      VK_FORMAT_R16G16B16_UINT      },
        { Format::R16G16B16_SINT,      VK_FORMAT_R16G16B16_SINT      },
        { Format::R16G16B16_SFLOAT,    VK_FORMAT_R16G16B16_SFLOAT    },
        { Format::R16G16B16A16_UNORM,  VK_FORMAT_R16G16B16A16_UNORM  },
        { Format::R16G16B16A16_SNORM,  VK_FORMAT_R16G16B16A16_SNORM  },
        { Format::R16G16B16A16_UINT,   VK_FORMAT_R16G16B16A16_UINT   },
        { Format::R16G16B16A16_SINT,   VK_FORMAT_R16G16B16A16_SINT   },
        { Format::R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },

        { Format::R32_UINT,            VK_FORMAT_R32_UINT            },
        { Format::R32_SINT,            VK_FORMAT_R32_SINT            },
        { Format::R32_SFLOAT,          VK_FORMAT_R32_SFLOAT          },
        { Format::R32G32_UINT,         VK_FORMAT_R32G32_UINT         },
        { Format::R32G32_SINT,         VK_FORMAT_R32G32_SINT         },
        { Format::R32G32_SFLOAT,       VK_FORMAT_R32G32_SFLOAT       },
        { Format::R32G32B32_UINT,      VK_FORMAT_R32G32B32_UINT      },
        { Format::R32G32B32_SINT,      VK_FORMAT_R32G32B32_SINT      },
        { Format::R32G32B32_SFLOAT,    VK_FORMAT_R32G32B32_SFLOAT    },
        { Format::R32G32B32A32_UINT,   VK_FORMAT_R32G32B32A32_UINT   },
        { Format::R32G32B32A32_SINT,   VK_FORMAT_R32G32B32A32_SINT   },
        { Format::R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT },

        { Format::R64_UINT,            VK_FORMAT_R64_UINT            },
        { Format::R64_SINT,            VK_FORMAT_R64_SINT            },
        { Format::R64_SFLOAT,          VK_FORMAT_R64_SFLOAT          },
        { Format::R64G64_UINT,         VK_FORMAT_R64G64_UINT         },
        { Format::R64G64_SINT,         VK_FORMAT_R64G64_SINT         },
        { Format::R64G64_SFLOAT,       VK_FORMAT_R64G64_SFLOAT       },
        { Format::R64G64B64_UINT,      VK_FORMAT_R64G64B64_UINT      },
        { Format::R64G64B64_SINT,      VK_FORMAT_R64G64B64_SINT      },
        { Format::R64G64B64_SFLOAT,    VK_FORMAT_R64G64B64_SFLOAT    },
        { Format::R64G64B64A64_UINT,   VK_FORMAT_R64G64B64A64_UINT   },
        { Format::R64G64B64A64_SINT,   VK_FORMAT_R64G64B64A64_SINT   },
        { Format::R64G64B64A64_SFLOAT, VK_FORMAT_R64G64B64A64_SFLOAT },

        { Format::S8_UINT,            VK_FORMAT_S8_UINT            },
        { Format::D16_UNORM,          VK_FORMAT_D16_UNORM          },
        { Format::D32_SFLOAT,         VK_FORMAT_D32_SFLOAT         },
        { Format::D16_UNORM_S8_UINT,  VK_FORMAT_D16_UNORM_S8_UINT  },
        { Format::D24_UNORM_S8_UINT,  VK_FORMAT_D24_UNORM_S8_UINT  },
        { Format::D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT },
    };

    constexpr u64 range = sizeof(lookup_map) / sizeof(lookup_map[0]);
    for (u32 i = 0; i < range; ++i) {
        if (lookup_map[i].first == format) {
            return lookup_map[i].second;
        }
    }

    LOG_ERROR("Failed to find VkFormat");
    return VK_FORMAT_UNDEFINED;
}

}

#endif // HK_VKUTILS_H
