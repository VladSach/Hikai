#ifndef HK_BACKEND_H
#define HK_BACKEND_H

#include "backend_types.h"

#include "utility/hktypes.h"
#include "hkstl/strings/hkstring.h"

namespace hk::platform {

// TODO: rename, since it's a backend init, not platform's
void init();
void deinit();

b8 process_messages();

// TODO:
// The design i'm thinking about right now is to have all platforms
// create a ROOT window invisible to user that belongs to the engine
// every user-created windows will be children of this root window
//
// OTHER, better idea: ROOT window still belongs to engine, but it is visible
// to user. At the beggining it is hidden, so usercode needs to "turn it on"
// but, if the user doesn't require more than one window, he won't ever have to
// think about creating one and will be provided with the default

/* ======================= WINDOWS ======================= */

struct WindowHandle
{
    u8 index : 6; // 64 windows seems enough
    u8 magic : 2;
};

HKAPI WindowHandle create_window(WindowDesc desc);

u32 window_width(WindowHandle handle);
u32 window_height(WindowHandle handle);

b8 window_visible(WindowHandle handle);

void window_show(WindowHandle handle);
void window_hide(WindowHandle handle);

// void window_set_title(WindowHandle handle, const char *title);
// void window_set_icon(WindowHandle handle, const char *path);

// void window_toggle_fullscreen(WindowHandle handle);

// void window_toggle_??? taskbar mode

/* ======================= Mouse settings ======================= */
HKAPI void hide_cursor();
HKAPI void show_cursor();

// Restrict cursor to area
HKAPI void lock_cursor(/*area*/);
HKAPI void unlock_cursor();

HKAPI void enable_raw_mouse_input();
HKAPI void disable_raw_mouse_input();

// Specs
void query_monitor_info();

// Utils
b8 copy_to_clipboard(const hk::string &target);

// HKAPI void create_popup(hk::string title);
HKAPI void create_message_box(const hk::string &title,
                              const hk::string &message);
HKAPI void create_task_dialog(const hk::string &title,
                              const hk::string &header,
                              const hk::string &message);

/* ======================= Rendering Backend ======================= */

// Depending on the backend this can be vulkan swapchain or software buffer
// void attach_swapchain();

// Vulkan
// void create_surface(void *instance, void *surface);

// Software Rendering (CPU)
// void attach_buffer(draw buffer, buffer_pos_x, buffer_pos_y);

}

#endif // HK_BACKEND_H
