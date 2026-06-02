#ifndef HK_X11_H
#define HK_X11_H

#include "platform/backend/backend.h"

#include "core/input.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xrandr.h>

namespace hk::platform::x11 {

// FIX: temp
Display* get_display();
Window get_window();

void parse_event(XEvent event);

i32 handle_error(Display *display, XErrorEvent *event);

input::Button map_keysym(KeySym keysym);

}

#endif // HK_X11_H
