#ifndef HK_WAYLAND_CONTEXT_H
#define HK_WAYLAND_CONTEXT_H

#include "wltypes.h"
#include "containers/hkvector.h"

namespace hk::platform::wl {

struct WaylandContext {
    // File Descriptor for Wayland socket
    i32 display = -1;

    // An ID of 0 represents a null value
    u32 curr_id = 1;

    // Display ID is stated by a standard to be always 1
    u32 wl_display = 1;    // core global object
    u32 wl_registry = 0;   // global registry object
    u32 wl_compositor = 0; // the compositor singleton
    u32 wl_shm = 0;        // shared memory support
    u32 wl_seat = 0;       // group of input devices

    // Peripherals
    u32 wl_pointer = 0;    // pointer input device
    u32 wl_keyboard = 0;   // keyboard input device
    u32 wl_touch = 0;      // touchscreen input device
    u32 wl_output = 0;     // compositor output region

    u32 xdg_wm_base = 0;

    struct WlSeat {
        const char* name = "";
        u32 capability = 0;
    } seat;

    struct WlOutput {
        const char *name = "";
        const char *model = "";
        const char *vendor = "";
        const char *desc = "";

        // Resolution
        u32 width = 0;
        u32 height = 0;

        // Refresh Rate
        u32 hz = 0;

        f32 scale = 1.f;

        // Color Depth
        u32 depth = 0;
	
        // Position within the global compositor space
        u32 x = 0;
        u32 y = 0;

        // Size in millimeters
        u32 physical_width = 0;
        u32 physical_height = 0;

        // Subpixel orientation
        wl::Subpixel subpixel = Subpixel::unknown;

        // Fransformation applied to buffer contents during presentation
        u32 transform = 0;
    };
    hk::vector<WlOutput> outputs;

    hk::vector<wl::Format> supported_formats;

    // Window specific
    u32 wl_surface = 0;             // an onscreen surface
    u32 wl_buffer = 0;              // content for a wl_surface
    u32 wl_shm_pool = 0;            // a shared memory pool
    u32 wl_data_offer = 0;          // offer to transfer data
    u32 wl_data_source = 0;         // offer to transfer data
    u32 wl_data_device = 0;         // data transfer device
    u32 wl_data_device_manager = 0; // data transfer interface
    u32 wl_subsurface = 0;          // sub-surface interface to a wl_surface

    u32 xdg_positioner = 0;
    u32 xdg_surface = 0;
    u32 xdg_toplevel = 0;
    u32 xdg_popup = 0;

    i32 shm = -1; // file descriptor
    u32 shm_pool_size = 0;
    void *shm_pool_data = nullptr;

    enum SurfaceState {
        NONE,
        ACKED_CONFIGURE,
        ATTACHED,
    } state = NONE;

    u32 width = 100;
    u32 height = 100;

    // Idea
    // std::map<wl_surface, Window> send_inputs;
};

}

#endif // HK_WAYLAND_CONTEXT_H
