#ifndef HK_BACKEND_H
#define HK_BACKEND_H

#include "utility/hktypes.h"

namespace hk::platform {

/* Platform Backend */
void init();
void deinit();

b8 process_messages();

// void create_popup(hk::string title);

// window
b8 create_window(const char *title, u32 width, u32 height);

// TODO: unsure about window design,
// do I want it to be it's own thing or only as a part of backend?
// this are here temp, until I figure it out

u32 window_width();
u32 window_height();

// void window_show();
// void window_hide();
//
// void window_set_title(const char *title);
// void window_set_icon(const char *path);
//
// void window_toggle_fullscreen();
//
//
// void window_toggle_??? taskbar mode


// Depending on the backend this can be vulkan swapchain or software buffer
// attach_swapchain();


// Specs
void query_monitor_info();

// Utils
// b8 copy_to_clipboard(const hk::string &target);
//
// void create_message_box(const hk::string &title,
//                         const hk::string &message);
// void create_task_dialog(const hk::string &title,
//                         const hk::string &header,
//                         const hk::string &message);

/* Rendering Backend */
// Software Rendering (CPU)
// void attach_buffer(draw buffer, buffer_pos_x, buffer_pos_y);

}

#endif // HK_BACKEND_H
