#ifndef HK_WIN32_WINDOW_H
#define HK_WIN32_WINDOW_H

#include "win.h"

#include "utility/hktypes.h"

namespace hk::platform::win32 {

// using window_callback =
//     std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)>;

using window_callback = LRESULT(*)(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

struct Window {
    enum class State {
        MAXIMIZED,
        MINIMIZED,
        RESTORED
    } state = State::RESTORED;

    b8 is_fullscreen = false;

    HWND hwnd = nullptr;
    // HWND parent = nullptr;  // nullptr = top-level

    window_callback proc = nullptr;
};

} // hk::platform::win32

#endif // HK_WIN32_WINDOW_H
