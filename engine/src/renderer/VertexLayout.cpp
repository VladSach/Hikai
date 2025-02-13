#include "VertexLayout.h"

#include "math/hkmath.h"

#include <utility>

namespace hk {

enum class AllFormats : u16;
constexpr VkFormat getVkFormat(AllFormats value);

hk::vector<VkVertexInputAttributeDescription>
createVertexLayout(const hk::vector<hk::bitflag<Format>> &formats)
{
    u32 offset = 0;
    u32 bits = 0;
    u32 vec = 0;
    hk::vector<VkVertexInputAttributeDescription> attributeDescs;

    for (u32 i = 0; i < formats.size(); i++) {
        hk::bitflag<Format> format = formats[i];

        attributeDescs.push_back(
            {
                i, 0,
                getVkFormat(static_cast<AllFormats>(format.value())),
                offset
            }
        );

        if      (format & Format::B8)  bits = 8;
        else if (format & Format::B16) bits = 16;
        else if (format & Format::B32) bits = 32;
        else if (format & Format::B64) bits = 64;

        if      (format & Format::VEC1) vec = 1;
        else if (format & Format::VEC2) vec = 2;
        else if (format & Format::VEC3) vec = 3;
        else if (format & Format::VEC4) vec = 4;

        offset += (vec * bits) / 8;
    }

    return attributeDescs;
}

enum class AllFormats : u16 {
    UNDEFINED = 0,

    R8_UNORM            = 0b0000'1000'1010'0100,
    R8_SNORM            = 0b0000'1000'1010'0010,
    R8_UINT             = 0b0000'1000'1000'1100,
    R8_SINT             = 0b0000'1000'1000'1010,
    R8_SRGB             = 0b0000'1000'1100'0010,
    R8G8_UNORM          = 0b0001'0000'1010'0100,
    R8G8_SNORM          = 0b0001'0000'1010'0010,
    R8G8_UINT           = 0b0001'0000'1000'1100,
    R8G8_SINT           = 0b0001'0000'1000'1010,
    R8G8_SRGB           = 0b0001'0000'1100'0010,
    R8G8B8_UNORM        = 0b0010'0000'1010'0100,
    R8G8B8_SNORM        = 0b0010'0000'1010'0010,
    R8G8B8_UINT         = 0b0010'0000'1000'1100,
    R8G8B8_SINT         = 0b0010'0000'1000'1010,
    R8G8B8_SRGB         = 0b0010'0000'1100'0010,
    R8G8B8A8_UNORM      = 0b0100'0000'1010'0100,
    R8G8B8A8_SNORM      = 0b0100'0000'1010'0010,
    R8G8B8A8_UINT       = 0b0100'0000'1000'1100,
    R8G8B8A8_SINT       = 0b0100'0000'1000'1010,
    R8G8B8A8_SRGB       = 0b0100'0000'1100'0010,

    R16_UNORM           = 0b0000'1001'0010'0100,
    R16_SNORM           = 0b0000'1001'0010'0010,
    R16_UINT            = 0b0000'1001'0000'1100,
    R16_SINT            = 0b0000'1001'0000'1010,
    R16_SFLOAT          = 0b0000'1001'0001'0010,
    R16G16_UNORM        = 0b0001'0001'0010'0100,
    R16G16_SNORM        = 0b0001'0001'0010'0010,
    R16G16_UINT         = 0b0001'0001'0000'1100,
    R16G16_SINT         = 0b0001'0001'0000'1010,
    R16G16_SFLOAT       = 0b0001'0001'0001'0010,
    R16G16B16_UNORM     = 0b0010'0001'0010'0100,
    R16G16B16_SNORM     = 0b0010'0001'0010'0010,
    R16G16B16_UINT      = 0b0010'0001'0000'1100,
    R16G16B16_SINT      = 0b0010'0001'0000'1010,
    R16G16B16_SFLOAT    = 0b0010'0001'0001'0010,
    R16G16B16A16_UNORM  = 0b0100'0001'0010'0100,
    R16G16B16A16_SNORM  = 0b0100'0001'0010'0010,
    R16G16B16A16_UINT   = 0b0100'0001'0000'1100,
    R16G16B16A16_SINT   = 0b0100'0001'0000'1010,
    R16G16B16A16_SFLOAT = 0b0100'0001'0001'0010,

    R32_UINT            = 0b0000'1010'0000'1100,
    R32_SINT            = 0b0000'1010'0000'1010,
    R32_SFLOAT          = 0b0000'1010'0001'0010,
    R32G32_UINT         = 0b0001'0010'0000'1100,
    R32G32_SINT         = 0b0001'0010'0000'1010,
    R32G32_SFLOAT       = 0b0001'0010'0001'0010,
    R32G32B32_UINT      = 0b0010'0010'0000'1100,
    R32G32B32_SINT      = 0b0010'0010'0000'1010,
    R32G32B32_SFLOAT    = 0b0010'0010'0001'0010,
    R32G32B32A32_UINT   = 0b0100'0010'0000'1100,
    R32G32B32A32_SINT   = 0b0100'0010'0000'1010,
    R32G32B32A32_SFLOAT = 0b0100'0010'0001'0010,

    R64_UINT            = 0b0000'1100'0000'1100,
    R64_SINT            = 0b0000'1100'0000'1010,
    R64_SFLOAT          = 0b0000'1100'0001'0010,
    R64G64_UINT         = 0b0001'0100'0000'1100,
    R64G64_SINT         = 0b0001'0100'0000'1010,
    R64G64_SFLOAT       = 0b0001'0100'0001'0010,
    R64G64B64_UINT      = 0b0010'0100'0000'1100,
    R64G64B64_SINT      = 0b0010'0100'0000'1010,
    R64G64B64_SFLOAT    = 0b0010'0100'0001'0010,
    R64G64B64A64_UINT   = 0b0100'0100'0000'1100,
    R64G64B64A64_SINT   = 0b0100'0100'0000'1010,
    R64G64B64A64_SFLOAT = 0b0100'0100'0001'0010,
};

constexpr VkFormat getVkFormat(AllFormats key) {
    constexpr std::pair<AllFormats, VkFormat> VkFormatsMap[] =
    {
        { AllFormats::UNDEFINED,      VK_FORMAT_UNDEFINED },

        { AllFormats::R8_UNORM,       VK_FORMAT_R8_UNORM       },
        { AllFormats::R8_SNORM,       VK_FORMAT_R8_SNORM       },
        { AllFormats::R8_UINT,        VK_FORMAT_R8_UINT        },
        { AllFormats::R8_SINT,        VK_FORMAT_R8_SINT        },
        { AllFormats::R8_SRGB,        VK_FORMAT_R8_SRGB        },
        { AllFormats::R8G8_UNORM,     VK_FORMAT_R8G8_UNORM     },
        { AllFormats::R8G8_SNORM,     VK_FORMAT_R8G8_SNORM     },
        { AllFormats::R8G8_UINT,      VK_FORMAT_R8G8_UINT      },
        { AllFormats::R8G8_SINT,      VK_FORMAT_R8G8_SINT      },
        { AllFormats::R8G8_SRGB,      VK_FORMAT_R8G8_SRGB      },
        { AllFormats::R8G8B8_UNORM,   VK_FORMAT_R8G8B8_UNORM   },
        { AllFormats::R8G8B8_SNORM,   VK_FORMAT_R8G8B8_SNORM   },
        { AllFormats::R8G8B8_UINT,    VK_FORMAT_R8G8B8_UINT    },
        { AllFormats::R8G8B8_SINT,    VK_FORMAT_R8G8B8_SINT    },
        { AllFormats::R8G8B8_SRGB,    VK_FORMAT_R8G8B8_SRGB    },
        { AllFormats::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
        { AllFormats::R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM },
        { AllFormats::R8G8B8A8_UINT,  VK_FORMAT_R8G8B8A8_UINT  },
        { AllFormats::R8G8B8A8_SINT,  VK_FORMAT_R8G8B8A8_SINT  },
        { AllFormats::R8G8B8A8_SRGB,  VK_FORMAT_R8G8B8A8_SRGB  },

        { AllFormats::R16_UNORM,           VK_FORMAT_R16_UNORM           },
        { AllFormats::R16_SNORM,           VK_FORMAT_R16_SNORM           },
        { AllFormats::R16_UINT,            VK_FORMAT_R16_UINT            },
        { AllFormats::R16_SINT,            VK_FORMAT_R16_SINT            },
        { AllFormats::R16_SFLOAT,          VK_FORMAT_R16_SFLOAT          },
        { AllFormats::R16G16_UNORM,        VK_FORMAT_R16G16_UNORM        },
        { AllFormats::R16G16_SNORM,        VK_FORMAT_R16G16_SNORM        },
        { AllFormats::R16G16_UINT,         VK_FORMAT_R16G16_UINT         },
        { AllFormats::R16G16_SINT,         VK_FORMAT_R16G16_SINT         },
        { AllFormats::R16G16_SFLOAT,       VK_FORMAT_R16G16_SFLOAT       },
        { AllFormats::R16G16B16_UNORM,     VK_FORMAT_R16G16B16_UNORM     },
        { AllFormats::R16G16B16_SNORM,     VK_FORMAT_R16G16B16_SNORM     },
        { AllFormats::R16G16B16_UINT,      VK_FORMAT_R16G16B16_UINT      },
        { AllFormats::R16G16B16_SINT,      VK_FORMAT_R16G16B16_SINT      },
        { AllFormats::R16G16B16_SFLOAT,    VK_FORMAT_R16G16B16_SFLOAT    },
        { AllFormats::R16G16B16A16_UNORM,  VK_FORMAT_R16G16B16A16_UNORM  },
        { AllFormats::R16G16B16A16_SNORM,  VK_FORMAT_R16G16B16A16_SNORM  },
        { AllFormats::R16G16B16A16_UINT,   VK_FORMAT_R16G16B16A16_UINT   },
        { AllFormats::R16G16B16A16_SINT,   VK_FORMAT_R16G16B16A16_SINT   },
        { AllFormats::R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT },

        { AllFormats::R32_UINT,            VK_FORMAT_R32_UINT            },
        { AllFormats::R32_SINT,            VK_FORMAT_R32_SINT            },
        { AllFormats::R32_SFLOAT,          VK_FORMAT_R32_SFLOAT          },
        { AllFormats::R32G32_UINT,         VK_FORMAT_R32G32_UINT         },
        { AllFormats::R32G32_SINT,         VK_FORMAT_R32G32_SINT         },
        { AllFormats::R32G32_SFLOAT,       VK_FORMAT_R32G32_SFLOAT       },
        { AllFormats::R32G32B32_UINT,      VK_FORMAT_R32G32B32_UINT      },
        { AllFormats::R32G32B32_SINT,      VK_FORMAT_R32G32B32_SINT      },
        { AllFormats::R32G32B32_SFLOAT,    VK_FORMAT_R32G32B32_SFLOAT    },
        { AllFormats::R32G32B32A32_UINT,   VK_FORMAT_R32G32B32A32_UINT   },
        { AllFormats::R32G32B32A32_SINT,   VK_FORMAT_R32G32B32A32_SINT   },
        { AllFormats::R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT },

        { AllFormats::R64_UINT,            VK_FORMAT_R64_UINT            },
        { AllFormats::R64_SINT,            VK_FORMAT_R64_SINT            },
        { AllFormats::R64_SFLOAT,          VK_FORMAT_R64_SFLOAT          },
        { AllFormats::R64G64_UINT,         VK_FORMAT_R64G64_UINT         },
        { AllFormats::R64G64_SINT,         VK_FORMAT_R64G64_SINT         },
        { AllFormats::R64G64_SFLOAT,       VK_FORMAT_R64G64_SFLOAT       },
        { AllFormats::R64G64B64_UINT,      VK_FORMAT_R64G64B64_UINT      },
        { AllFormats::R64G64B64_SINT,      VK_FORMAT_R64G64B64_SINT      },
        { AllFormats::R64G64B64_SFLOAT,    VK_FORMAT_R64G64B64_SFLOAT    },
        { AllFormats::R64G64B64A64_UINT,   VK_FORMAT_R64G64B64A64_UINT   },
        { AllFormats::R64G64B64A64_SINT,   VK_FORMAT_R64G64B64A64_SINT   },
        { AllFormats::R64G64B64A64_SFLOAT, VK_FORMAT_R64G64B64A64_SFLOAT },
    };

    constexpr u64 range = sizeof(VkFormatsMap) / sizeof(VkFormatsMap[0]);
    for (u32 i = 0; i < range; ++i) {
        if (VkFormatsMap[i].first == key) {
            return VkFormatsMap[i].second;
        }
    }

    LOG_ERROR("Failed to find VkFormat");
    return VK_FORMAT_UNDEFINED;
}

}
