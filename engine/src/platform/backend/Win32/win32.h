#ifndef HK_WIN32_h
#define HK_WIN32_h

#include "win.h"
#include "platform/backend/backend.h"

#include "win32window.h"

namespace hk::platform::win32 {

HINSTANCE get_instance();

HWND get_hwnd();

void set_proc(WindowHandle window, window_callback callback);

b8 prehook(Window *window, UINT msg, WPARAM wp, LPARAM lp);

}

// Routs messaged to windows
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

// Callback handler for the task dialog
HRESULT CALLBACK TaskDialogCallback(HWND hwndFocus, UINT uNotification,
                                    WPARAM wParam, LPARAM lParam,
                                    LONG_PTR dwRefData);

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData);


#endif // HK_WIN32_h
