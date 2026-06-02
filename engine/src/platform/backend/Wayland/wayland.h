#ifndef HK_WAYLAND_H
#define HK_WAYLAND_H

#include "platform/backend/backend.h"

#include "wltypes.h"
#include "wlutils.h"
#include "wlcontext.h"

namespace hk::platform::wl {

// Returns file descriptor for wayland display socket
i32 display_connect();

b8 display_roundtrip();

b8 peek_message(wire_message <4096> &msg);

// Creates new wayland object and increments id
void create_wayland_object(u32 object_id, u16 opcode);

void create_shm(u64 size);

/* ====== Wayland Requests ====== */

// Returns ObjectID of registry
u32 wl_display_get_registry();

u32 wl_registry_bind(u32 name, char *interface, u32 interface_len, u32 version);

u32 wl_compositor_create_surface();

u32 wl_shm_pool_create_buffer();

u32 wl_shm_create_pool();

void wl_surface_attach();

void wl_surface_commit();

void wl_seat_get_pointer();

void wl_seat_get_keyboard();

// Assigns wl_surface to the xdg_surface role
u32 xdg_wm_base_get_xdg_surface();

void xdg_wm_base_pong(u32 ping);

// Assigns xdg_surface to the xdg_toplevel role
u32 xdg_surface_get_toplevel();

void xdg_surface_ack_configure(u32 configure);

// void xdg_toplevel_set_parent();

void xdg_toplevel_set_title(const char *title);

void xdg_toplevel_set_app_id(const char *id);

void xdg_toplevel_set_max_size(u32 width, u32 height);

void xdg_toplevel_set_maximized();

void xdg_toplevel_unset_maximized();

void xdg_toplevel_set_fullscreen();

void xdg_toplevel_unset_fullscreen();

void xdg_toplevel_set_minimized();

/* ====== Wayland Events ====== */

void wl_display_error(wire_message<4096> &msg);

void wl_registry_global(wire_message<4096> &msg);

}

#endif // HK_WAYLAND_H
