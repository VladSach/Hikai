#ifndef HK_WIN32_CONTEXT_H
#define HK_WIN32_CONTEXT_H

#include "win32.h"
#include "win32window.h"

namespace hk::platform::win32 {

struct Context {
    HINSTANCE inst = nullptr;

    b8 is_cursor_enabled = true;
    b8 is_raw_mouse_input_enabled = false;

    hk::vector<Window> windows;
};

}

#endif // HK_WIN32_CONTEXT_H
