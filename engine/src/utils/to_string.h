#ifndef HK_TO_STRING_H
#define HK_TO_STRING_H

#include "spec_types.h"
#include "core/input.h"
#include "hkstl/Filewatch.h"
#include "renderer/resources.h"
#include "resources/Asset.h"

namespace hk {

namespace spec {

constexpr const char* to_string(AdapterType type)
{
    constexpr const char* lookup_type[] = {
        "Other",
        "Integrated GPU",
        "Discrete GPU",
        "Virtual GPU",
        "CPU",
    };

    return lookup_type[static_cast<u32>(type)];
}

constexpr const char* to_string(AdapterVendor vendor)
{
    constexpr const char* lookup_vendor[] = {
        "Other",
        "AMD",
        "NVidia",
        "Intel",
        "ARM",
        "Qualcomm",
        "MAX VENDOR",
    };

    return lookup_vendor[static_cast<u32>(vendor)];
}

constexpr const char* to_string(BackendType type) {
    constexpr const char* lookup_type[] = {
        "OpenGL",
        "Vulkan",
        "DirectX11",
        "DirectX12",
    };

    return lookup_type[static_cast<u32>(type)];
}

} // namespace spec

namespace log {

constexpr char const* to_string(Level level)
{
    constexpr char const *lookup_level[] =
    {
        "[FATAL]:",
        "[ERROR]:",
        "[WARN]:",
        "[INFO]:",
        "[DEBUG]:",
        "[TRACE]:"
    };

    return lookup_level[static_cast<u32>(level)];
}

} // namespace log

namespace filewatch {

constexpr const char* to_string(State state)
{
    constexpr const char *lookup_file_state[] = {
        "NONE",

        "ADDED",
        "REMOVED",
        "MODIFIED",
        "RENAMED_OLD",
        "RENAMED_NEW",

        "MAX_FILE_STATE"
    };

    return lookup_file_state[static_cast<u32>(state)];
}

} // namespace filewatch

namespace event {

constexpr const char* to_string(EventCode code)
{
    constexpr char const *lookup_events[MAX_EVENT_CODES] = {
        "Empty Event",

        "App Shutdown Event",

        "Key Pressed Event",
        "Key Released Event",

        "Mouse Pressed Event",
        "Mouse Released Event",

        "Mouse Moved Event",

        "Raw Mouse Moved Event",

        "Mouse Wheel Event",

        "Window Resized Event",

        "Asset Loaded Event",
    };

    return lookup_events[code];
}

constexpr const char* to_string(ErrorCode error)
{
    constexpr char const *lookup_errors[MAX_ERROR_CODES] = {
        "Unknown error",

        "Unsupported Graphics API",
    };

    return lookup_errors[error];
}

} // namespace event

namespace input {

constexpr const char* to_string(Button button)
{
    constexpr char const *NDF = "Undefined";
    constexpr char const *lookup_keys[MAX_KEYS] = {
        "Empty",

        "Mouse Left",
        "Mouse Right",
        NDF, // 0x03
        "Mouse Middle",

        // 0x05 - 0x07
        NDF, NDF, NDF,

        "Backspace",
        "Tab",

        // 0x0A - 0x0C
        NDF, NDF, NDF,

        "Enter",

        // 0x0E - 0x0F
        NDF, NDF,

        "Shift",
        "Ctrl",
        "Alt",
        "Pause",
        "Caps Lock",

        // 0x15 - 0x1A
        NDF, NDF, NDF, NDF, NDF, NDF,

        "Esc",

        // 0x1C - 0x1F
        NDF, NDF, NDF, NDF,

        "Space",
        "Page Up",
        "Page Down",
        "End",
        "Home",
        "Left",
        "Up",
        "Right",
        "Down",
        "Select",
        "Print",
        "Execute",
        "Print Screen",
        "Insert",
        "Delete",
        "Help",

        "0", "1", "2", "3", "4",
        "5", "6", "7", "8", "9",

        // 0x3A - 0x40
        NDF, NDF, NDF,
        NDF, NDF, NDF, NDF,

        "A", "B", "C", "D", "E", "F",
        "G", "H", "I", "J", "K", "L",
        "M", "N", "O", "P", "Q", "R",
        "S", "T", "U", "V", "W", "X",
        "Y", "Z",

        "Left Super",
        "Right Super",
        "Apps",

        // 0x5E
        NDF,

        "Sleep",

        "Numpad 0",
        "Numpad 1",
        "Numpad 2",
        "Numpad 3",
        "Numpad 4",
        "Numpad 5",
        "Numpad 6",
        "Numpad 7",
        "Numpad 8",
        "Numpad 9",

        "Numpad Mult",
        "Numpad Add",
        "Numpad Sep",
        "Numpad Sub",
        "Numpad Dec",
        "Numpad Div",

        "F1", "F2", "F3", "F4", "F5",
        "F6", "F7", "F7", "F8", "F9",
        "F10", "F11", "F12", "F13", "F14",
        "F15", "F16", "F17", "F18", "F19",
        "F20", "F21", "F22", "F23", "F24",

        // 0x88 - 0x8F
        NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF,

        "Num Lock",
        "Scroll Lock",

        // 0x92 - 0x9F
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF,

        "Left Shift",
        "Right Shift",
        "Left Ctrl",
        "Right Ctrl",
        "Left Alt",
        "Right Alt",

        // 0xA6 - 0xB9
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,

        ":;",
        "=+",
        ",",
        "-_",
        ".",
        "/?",
        "~`",

        // 0xC1 - 0xDA
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF,

        "[{",
        "\\|",
        "}]",

        "\'\"",
    };
    return lookup_keys[button];
}

} // namespace input

namespace asset {

constexpr char const* to_string(const Asset::Type &type)
{
    constexpr char const *lookup_types[] = {
        "None",

        "Texture",
        "Shader",
        "Mesh",
        "Material",
        "Model",

        "Max",
    };

    return lookup_types[static_cast<u32>(type)];
}

} // namespace asset

constexpr char const* to_string(MemoryType type)
{
    constexpr char const *lookup_type[] = {
        "None",
        "GPU Local",
        "CPU Upload",
        "CPU Readback",
    };
    return lookup_type[static_cast<u32>(type)];
}

constexpr char const* to_string(BufferType type)
{
    constexpr char const *lookup_type[] = {
        "None",
        "Vertex Buffer",
        "Index Buffer",
        "Uniform Buffer",
        "Max Buffer Type",
    };
    return lookup_type[static_cast<u32>(type)];
}

constexpr char const* to_string(ImageType type)
{
    constexpr char const *lookup_type[] = {
        "None",
        "Texture",
        "Render Target",
        "Depth Buffer",
        "Max Image Type",
    };
    return lookup_type[static_cast<u32>(type)];
}

constexpr char const* to_string(Format format)
{
    constexpr std::pair<Format, char const *> lookup_map[] =
    {
        { Format::UNDEFINED,      "Undefined" },

        { Format::R8_UNORM,       "R8_UNORM"       },
        { Format::R8_SNORM,       "R8_SNORM"       },
        { Format::R8_UINT,        "R8_UINT"        },
        { Format::R8_SINT,        "R8_SINT"        },
        { Format::R8_SRGB,        "R8_SRGB"        },
        { Format::R8G8_UNORM,     "R8G8_UNORM"     },
        { Format::R8G8_SNORM,     "R8G8_SNORM"     },
        { Format::R8G8_UINT,      "R8G8_UINT"      },
        { Format::R8G8_SINT,      "R8G8_SINT"      },
        { Format::R8G8_SRGB,      "R8G8_SRGB"      },
        { Format::R8G8B8_UNORM,   "R8G8B8_UNORM"   },
        { Format::R8G8B8_SNORM,   "R8G8B8_SNORM"   },
        { Format::R8G8B8_UINT,    "R8G8B8_UINT"    },
        { Format::R8G8B8_SINT,    "R8G8B8_SINT"    },
        { Format::R8G8B8_SRGB,    "R8G8B8_SRGB"    },
        { Format::R8G8B8A8_UNORM, "R8G8B8A8_UNORM" },
        { Format::R8G8B8A8_SNORM, "R8G8B8A8_SNORM" },
        { Format::R8G8B8A8_UINT,  "R8G8B8A8_UINT"  },
        { Format::R8G8B8A8_SINT,  "R8G8B8A8_SINT"  },
        { Format::R8G8B8A8_SRGB,  "R8G8B8A8_SRGB"  },

        { Format::R16_UNORM,           "R16_UNORM"           },
        { Format::R16_SNORM,           "R16_SNORM"           },
        { Format::R16_UINT,            "R16_UINT"            },
        { Format::R16_SINT,            "R16_SINT"            },
        { Format::R16_SFLOAT,          "R16_SFLOAT"          },
        { Format::R16G16_UNORM,        "R16G16_UNORM"        },
        { Format::R16G16_SNORM,        "R16G16_SNORM"        },
        { Format::R16G16_UINT,         "R16G16_UINT"         },
        { Format::R16G16_SINT,         "R16G16_SINT"         },
        { Format::R16G16_SFLOAT,       "R16G16_SFLOAT"       },
        { Format::R16G16B16_UNORM,     "R16G16B16_UNORM"     },
        { Format::R16G16B16_SNORM,     "R16G16B16_SNORM"     },
        { Format::R16G16B16_UINT,      "R16G16B16_UINT"      },
        { Format::R16G16B16_SINT,      "R16G16B16_SINT"      },
        { Format::R16G16B16_SFLOAT,    "R16G16B16_SFLOAT"    },
        { Format::R16G16B16A16_UNORM,  "R16G16B16A16_UNORM"  },
        { Format::R16G16B16A16_SNORM,  "R16G16B16A16_SNORM"  },
        { Format::R16G16B16A16_UINT,   "R16G16B16A16_UINT"   },
        { Format::R16G16B16A16_SINT,   "R16G16B16A16_SINT"   },
        { Format::R16G16B16A16_SFLOAT, "R16G16B16A16_SFLOAT" },

        { Format::R32_UINT,            "R32_UINT"            },
        { Format::R32_SINT,            "R32_SINT"            },
        { Format::R32_SFLOAT,          "R32_SFLOAT"          },
        { Format::R32G32_UINT,         "R32G32_UINT"         },
        { Format::R32G32_SINT,         "R32G32_SINT"         },
        { Format::R32G32_SFLOAT,       "R32G32_SFLOAT"       },
        { Format::R32G32B32_UINT,      "R32G32B32_UINT"      },
        { Format::R32G32B32_SINT,      "R32G32B32_SINT"      },
        { Format::R32G32B32_SFLOAT,    "R32G32B32_SFLOAT"    },
        { Format::R32G32B32A32_UINT,   "R32G32B32A32_UINT"   },
        { Format::R32G32B32A32_SINT,   "R32G32B32A32_SINT"   },
        { Format::R32G32B32A32_SFLOAT, "R32G32B32A32_SFLOAT" },

        { Format::R64_UINT,            "R64_UINT"            },
        { Format::R64_SINT,            "R64_SINT"            },
        { Format::R64_SFLOAT,          "R64_SFLOAT"          },
        { Format::R64G64_UINT,         "R64G64_UINT"         },
        { Format::R64G64_SINT,         "R64G64_SINT"         },
        { Format::R64G64_SFLOAT,       "R64G64_SFLOAT"       },
        { Format::R64G64B64_UINT,      "R64G64B64_UINT"      },
        { Format::R64G64B64_SINT,      "R64G64B64_SINT"      },
        { Format::R64G64B64_SFLOAT,    "R64G64B64_SFLOAT"    },
        { Format::R64G64B64A64_UINT,   "R64G64B64A64_UINT"   },
        { Format::R64G64B64A64_SINT,   "R64G64B64A64_SINT"   },
        { Format::R64G64B64A64_SFLOAT, "R64G64B64A64_SFLOAT" },

        { Format::S8_UINT,            "S8_UINT"            },
        { Format::D16_UNORM,          "D16_UNORM"          },
        { Format::D32_SFLOAT,         "D32_SFLOAT"         },
        { Format::D16_UNORM_S8_UINT,  "D16_UNORM_S8_UINT"  },
        { Format::D24_UNORM_S8_UINT,  "D24_UNORM_S8_UINT"  },
        { Format::D32_SFLOAT_S8_UINT, "D32_SFLOAT_S8_UINT" },
    };
    constexpr u64 range = sizeof(lookup_map) / sizeof(lookup_map[0]);
    for (u32 i = 0; i < range; ++i) {
        if (lookup_map[i].first == format) {
            return lookup_map[i].second;
        }
    }

    return "Undefined";
}

// constexpr char const* to_string(Setting setting)


} // namespace hk

#endif // HK_TO_STRING_H
