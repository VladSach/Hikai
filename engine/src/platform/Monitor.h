#ifndef HK_MONITOR_H
#define HK_MONITOR_H

#include "defines.h"
#include "platform.h"
#include "utils/containers/hkvector.h"

namespace hk::platform {

struct MonitorInfo {
    std::string name;

    // resolution
    u32 width = 0;
    u32 height = 0;

    f32 dpi = 0.f;

    // Refresh rate
    u32 hz = 0;

    // color depth
    u32 depth = 0;
};

HKAPI void getMonitors();

hk::vector<MonitorInfo> &getMonitorInfos();

}

#endif // HK_MONITOR_H
