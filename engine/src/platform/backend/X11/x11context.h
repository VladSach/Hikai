#ifndef HK_X11_CONTEXT_H
#define HK_X11_CONTEXT_H

#include "x11.h"

namespace hk::platform::x11 {

struct X11Context {
    Display *display = nullptr;

    b8 is_xwayland = false;

    Window root = 0;
    Window window = 0;

    Atom wm_delete = 0;


    u32 win_width = 0;
    u32 win_height = 0;
};

}

#endif // HK_X11_CONTEXT_H
