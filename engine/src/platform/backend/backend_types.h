#ifndef HK_BACKEND_TYPES_H
#define HK_BACKEND_TYPES_H

#include "hkstl/utility/hktypes.h"
#include "hkstl/strings/hkstring.h"

namespace hk::platform {

struct WindowDesc {
    hk::string title = "";
    u32 width = 400;
    u32 height = 400;
    u32 x = 0;
    u32 y = 0;
};

struct MonitorSpec {
    hk::string name;
    hk::string vendor;
    hk::string model;

    // Resolution
    u32 width = 0;
    u32 height = 0;

    f32 scale = 1.f;

    // Refresh Rate
    u32 hz = 0;

    // Color Depth
    u32 depth = 0;

    // Position
    u32 x = 0;
    u32 y = 0;

    // Physical demensions in mm
    u32 physical_width = 0;
    u32 physical_height = 0;
};

struct SystemSpec {
    enum class SystemType : u8 {
        WINDOWS,
        LINUX,
    } type;

    hk::vector<MonitorSpec> monitors;
    // Mouse
    // Keyboard
    // Gamepad
    // etc
};

} // hk::pltf

#endif // HK_BACKEND_TYPES_H
