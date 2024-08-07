#include "platform/Monitor.h"

namespace hk::platform {

HKAPI BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData);

static hk::vector<MonitorInfo> infos;

void getMonitors()
{
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    u32 deviceIndex = 0;
    while (EnumDisplayDevices(0, deviceIndex, &dd, 0)) {
        std::string deviceName = dd.DeviceName;
        u32 monitorIndex = 0;
        while (EnumDisplayDevices(deviceName.c_str(), monitorIndex, &dd, 0)) {
            infos[monitorIndex].name = dd.DeviceName;
            infos[monitorIndex].name += ", ";
            infos[monitorIndex].name += dd.DeviceString;
            ++monitorIndex;
        }
        ++deviceIndex;
    }
}

hk::vector<MonitorInfo> &getMonitorInfos()
{
    return infos;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor, (void)lprcMonitor, (void)dwData;

    hk::platform::MonitorInfo out;

    /* Source:
     * https://stackoverflow.com/questions/70976583/
     * get-real-screen-resolution-using-win32-api
     */

    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(hMonitor, &info);

    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);

    // It returns native resolution in any case,
    // even if the OS tries to lie due to the DPI awareness of the process
    out.width = devmode.dmPelsWidth;
    out.height = devmode.dmPelsHeight;

    out.dpi = static_cast<f32>(GetDpiForSystem()) / 96.f /
              ((info.rcMonitor.right - info.rcMonitor.left) /
              static_cast<f32>(devmode.dmPelsWidth));

    out.hz = devmode.dmDisplayFrequency;
    out.depth = devmode.dmBitsPerPel;

    infos.push_back(out);
    return TRUE;
}

}

