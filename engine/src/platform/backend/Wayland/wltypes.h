#ifndef HK_WAYLAND_TYPES_H
#define HK_WAYLAND_TYPES_H

#include "utility/hktypes.h"

namespace hk::platform::wl {

enum class opcode : u16 {
    // Requests (client to server messages)
    wl_display_sync              = 0,
    wl_display_get_registry      = 1,
    wl_registry_bind             = 0,
    wl_compositor_create_surface = 0,
    wl_shm_pool_create_buffer    = 0,
    wl_shm_create_pool           = 0,
    wl_surface_attach            = 1,
    wl_surface_commit            = 6,
    wl_seat_get_pointer          = 0,
    wl_seat_get_keyboard         = 1,
    wl_seat_get_touch            = 2,

    xdg_wm_base_get_xdg_surface   = 2,
    xdg_wm_base_pong              = 3,
    xdg_surface_get_toplevel      = 1,
    xdg_surface_ack_configure     = 4,
    xdg_toplevel_set_title        = 2,
    xdg_toplevel_set_app_id       = 3,
    xdg_toplevel_set_max_size     = 7,
    xdg_toplevel_set_maximized    = 9,
    xdg_toplevel_unset_maximized  = 10,
    xdg_toplevel_set_fullscreen   = 11,
    xdg_toplevel_unset_fullscreen = 12,
    xdg_toplevel_set_minimized    = 13,

    // Events (server to client messages)
    wl_display_error       = 0,
    wl_registry_global     = 0,
    wl_shm_format          = 0,
    wl_buffer_release      = 0,
    wl_surface_enter       = 0,
    wl_surface_leave       = 1,
    wl_seat_capabilities   = 0,
    wl_seat_name           = 1,
    wl_output_geometry     = 0,
    wl_output_mode         = 1,
    wl_output_done         = 2,
    wl_output_scale        = 3,
    wl_output_name         = 4,
    wl_output_description  = 5,

    xdg_wm_base_ping             = 0,
    xdg_surface_configure        = 0,
    xdg_toplevel_configure       = 0,
    xdg_toplevel_close           = 1,
    xdg_toplevel_wm_capabilities = 3,

};

// Bitmask of capabilities this seat has
enum class Capability {
    pointer  = 1, // the seat has pointer devices
    keyboard = 2, // the seat has one or more keyboards
    touch    = 4, // the seat has touch devices
};

// How the physical pixels on an output are laid out
enum class Subpixel {
    unknown        = 0, // unknown geometry
    none           = 1, // no geometry
    horizontal_rgb = 2, // horizontal RGB
    horizontal_bgr = 3, // horizontal BGR
    vertical_rgb   = 4, // vertical RGB
    vertical_bgr   = 5, // vertical BGR
};

enum class Transform {
    normal      = 0, // no transform
    cc90        = 1, // 90 degrees counter-clockwise
    cc180       = 2, // 180 degrees counter-clockwise
    cc270       = 3, // 270 degrees counter-clockwise
    flipped     = 4, // 180 degree flip around a vertical axis
    flipped_90  = 5, // flip and rotate 90 degrees counter-clockwise
    flipped_180 = 6, // flip and rotate 180 degrees counter-clockwise
    flipped_270 = 7, // flip and rotate 270 degrees counter-clockwise
};

enum class Format {
    /* Guaranteed to be supported by the compositor per the specification */
    argb8888 = 0, // 32-bit ARGB format, [31:0] A:R:G:B 8:8:8:8 little endian
    xrgb8888 = 1, // 32-bit RGB format,  [31:0] x:R:G:B 8:8:8:8 little endian

    /* Other formats are optional */
    c8     = 0x20203843, // 8-bit color index format, [7:0] C
    rgb332 = 0x38424752, // 8-bit RGB format, [7:0] R:G:B 3:3:2
    bgr233 = 0x38524742, // 8-bit BGR format, [7:0] B:G:R 2:3:3

    xrgb4444 = 0x32315258, // 16-bit xRGB format, [15:0] x:R:G:B 4:4:4:4 little endian
    xbgr4444 = 0x32314258, // 16-bit xBGR format, [15:0] x:B:G:R 4:4:4:4 little endian
    rgbx4444 = 0x32315852, // 16-bit RGBx format, [15:0] R:G:B:x 4:4:4:4 little endian
    bgrx4444 = 0x32315842, // 16-bit BGRx format, [15:0] B:G:R:x 4:4:4:4 little endian
    argb4444 = 0x32315241, // 16-bit ARGB format, [15:0] A:R:G:B 4:4:4:4 little endian
    abgr4444 = 0x32314241, // 16-bit ABGR format, [15:0] A:B:G:R 4:4:4:4 little endian
    rgba4444 = 0x32314152, // 16-bit RBGA format, [15:0] R:G:B:A 4:4:4:4 little endian
    bgra4444 = 0x32314142, // 16-bit BGRA format, [15:0] B:G:R:A 4:4:4:4 little endian

    xrgb1555 = 0x35315258, // 16-bit xRGB      format, [15:0] x:R:G:B 1:5:5:5 little endian
    xbgr1555 = 0x35314258, // 16-bit xBGR 1555 format, [15:0] x:B:G:R 1:5:5:5 little endian
    rgbx5551 = 0x35315852, // 16-bit RGBx 5551 format, [15:0] R:G:B:x 5:5:5:1 little endian
    bgrx5551 = 0x35315842, // 16-bit BGRx 5551 format, [15:0] B:G:R:x 5:5:5:1 little endian
    argb1555 = 0x35315241, // 16-bit ARGB 1555 format, [15:0] A:R:G:B 1:5:5:5 little endian
    abgr1555 = 0x35314241, // 16-bit ABGR 1555 format, [15:0] A:B:G:R 1:5:5:5 little endian
    rgba5551 = 0x35314152, // 16-bit RGBA 5551 format, [15:0] R:G:B:A 5:5:5:1 little endian
    bgra5551 = 0x35314142, // 16-bit BGRA 5551 format, [15:0] B:G:R:A 5:5:5:1 little endian

    rgb565 = 0x36314752, // 16-bit RGB 565 format, [15:0] R:G:B 5:6:5 little endian
    bgr565 = 0x36314742, // 16-bit BGR 565 format, [15:0] B:G:R 5:6:5 little endian

    rgb888 = 0x34324752, // 24-bit RGB format, [23:0] R:G:B little endian
    bgr888 = 0x34324742, // 24-bit BGR format, [23:0] B:G:R little endian

    xbgr8888 = 0x34324258, // 32-bit xBGR format, [31:0] x:B:G:R 8:8:8:8 little endian
    rgbx8888 = 0x34325852, // 32-bit RGBx format, [31:0] R:G:B:x 8:8:8:8 little endian
    bgrx8888 = 0x34325842, // 32-bit BGRx format, [31:0] B:G:R:x 8:8:8:8 little endian
    abgr8888 = 0x34324241, // 32-bit ABGR format, [31:0] A:B:G:R 8:8:8:8 little endian
    rgba8888 = 0x34324152, // 32-bit RGBA format, [31:0] R:G:B:A 8:8:8:8 little endian
    bgra8888 = 0x34324142, // 32-bit BGRA format, [31:0] B:G:R:A 8:8:8:8 little endian

    xrgb2101010 = 0x30335258, // 32-bit xRGB format, [31:0] x:R:G:B 2:10:10:10 little endian
    xbgr2101010 = 0x30334258, // 32-bit xBGR format, [31:0] x:B:G:R 2:10:10:10 little endian
    rgbx1010102 = 0x30335852, // 32-bit RGBx format, [31:0] R:G:B:x 10:10:10:2 little endian
    bgrx1010102 = 0x30335842, // 32-bit BGRx format, [31:0] B:G:R:x 10:10:10:2 little endian
    argb2101010 = 0x30335241, // 32-bit ARGB format, [31:0] A:R:G:B 2:10:10:10 little endian
    abgr2101010 = 0x30334241, // 32-bit ABGR format, [31:0] A:B:G:R 2:10:10:10 little endian
    rgba1010102 = 0x30334152, // 32-bit RGBA format, [31:0] R:G:B:A 10:10:10:2 little endian
    bgra1010102 = 0x30334142, // 32-bit BGRA format, [31:0] B:G:R:A 10:10:10:2 little endian

    yuyv = 0x56595559, // packed  YCbCr format, [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian
    yvyu = 0x55595659, // packed  YCbCr format, [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian
    uyvy = 0x59565955, // packed  YCbCr format, [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian
    vyuy = 0x59555956, // packed  YCbCr format, [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian
    ayuv = 0x56555941, // packed AYCbCr format, [31:0] A:Y:Cb:Cr 8:8:8:8 little endian

    nv12  = 0x3231564e, // 2 plane YCbCr Cr:Cb format, 2x2 subsampled Cr:Cb plane
    nv21  = 0x3132564e, // 2 plane YCbCr Cb:Cr format, 2x2 subsampled Cb:Cr plane
    nv16  = 0x3631564e, // 2 plane YCbCr Cr:Cb format, 2x1 subsampled Cr:Cb plane
    nv61  = 0x3136564e, // 2 plane YCbCr Cb:Cr format, 2x1 subsampled Cb:Cr plane

    yuv410 = 0x39565559, // 3 plane YCbCr format, 4x4 subsampled Cb (1) and Cr (2) planes
    yvu410 = 0x39555659, // 3 plane YCbCr format, 4x4 subsampled Cr (1) and Cb (2) planes
    yuv411 = 0x31315559, // 3 plane YCbCr format, 4x1 subsampled Cb (1) and Cr (2) planes
    yvu411 = 0x31315659, // 3 plane YCbCr format, 4x1 subsampled Cr (1) and Cb (2) planes
    yuv420 = 0x32315559, // 3 plane YCbCr format, 2x2 subsampled Cb (1) and Cr (2) planes
    yvu420 = 0x32315659, // 3 plane YCbCr format, 2x2 subsampled Cr (1) and Cb (2) planes
    yuv422 = 0x36315559, // 3 plane YCbCr format, 2x1 subsampled Cb (1) and Cr (2) planes
    yvu422 = 0x36315659, // 3 plane YCbCr format, 2x1 subsampled Cr (1) and Cb (2) planes
    yuv444 = 0x34325559, // 3 plane YCbCr format, non-subsampled Cb (1) and Cr (2) planes
    yvu444 = 0x34325659, // 3 plane YCbCr format, non-subsampled Cr (1) and Cb (2) planes

    r8            = 0x20203852, // [7:0]  R
    r16           = 0x20363152, // [15:0] R little endian
    rg88          = 0x38384752, // [15:0] R:G 8:8 little endian
    gr88          = 0x38385247, // [15:0] G:R 8:8 little endian
    rg1616        = 0x32334752, // [31:0] R:G 16:16 little endian
    gr1616        = 0x32335247, // [31:0] G:R 16:16 little endian
    xrgb16161616f = 0x48345258, // [63:0] x:R:G:B 16:16:16:16 little endian
    xbgr16161616f = 0x48344258, // [63:0] x:B:G:R 16:16:16:16 little endian
    argb16161616f = 0x48345241, // [63:0] A:R:G:B 16:16:16:16 little endian
    abgr16161616f = 0x48344241, // [63:0] A:B:G:R 16:16:16:16 little endian

    xyuv8888        = 0x56555958, // [31:0] X:Y:Cb:Cr 8:8:8:8 little endian
    vuy888          = 0x34325556, // [23:0] Cr:Cb:Y 8:8:8 little endian
    vuy101010       = 0x30335556, // Y followed by U then V, 10:10:10. Non-linear modifier only

    y210            = 0x30313259, // [63:0] Cr0:0:Y1:0:Cb0:0:Y0:0 10:6:10:6:10:6:10:6 little endian per 2 Y pixels
    y212            = 0x32313259, // [63:0] Cr0:0:Y1:0:Cb0:0:Y0:0 12:4:12:4:12:4:12:4 little endian per 2 Y pixels
    y216            = 0x36313259, // [63:0] Cr0:Y1:Cb0:Y0 16:16:16:16 little endian per 2 Y pixels
    y410            = 0x30313459, // [31:0] A:Cr:Y:Cb 2:10:10:10 little endian
    y412            = 0x32313459, // [63:0] A:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian
    y416            = 0x36313459, // [63:0] A:Cr:Y:Cb 16:16:16:16 little endian

    xvyu2101010     = 0x30335658, // [31:0] X:Cr:Y:Cb 2:10:10:10 little endian
    xvyu12_16161616 = 0x36335658, // [63:0] X:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian
    xvyu16161616    = 0x38345658, // [63:0] X:Cr:Y:Cb 16:16:16:16 little endian

    y0l0            = 0x304c3059, // [63:0] A3:A2:Y3:0:Cr0:0:Y2:0:A1:A0:Y1:0:Cb0:0:Y0:0 1:1:8:2:8:2:8:2:1:1:8:2:8:2:8:2 little endian
    x0l0            = 0x304c3058, // [63:0] X3:X2:Y3:0:Cr0:0:Y2:0:X1:X0:Y1:0:Cb0:0:Y0:0 1:1:8:2:8:2:8:2:1:1:8:2:8:2:8:2 little endian
    y0l2            = 0x324c3059, // [63:0] A3:A2:Y3:Cr0:Y2:A1:A0:Y1:Cb0:Y0 1:1:10:10:10:1:1:10:10:10 little endian
    x0l2            = 0x324c3058, // [63:0] X3:X2:Y3:Cr0:Y2:X1:X0:Y1:Cb0:Y0 1:1:10:10:10:1:1:10:10:10 little endian

    yuv420_8bit  = 0x38305559,
    yuv420_10bit = 0x30315559,

    xrgb8888_a8 = 0x38415258,
    xbgr8888_a8 = 0x38414258,
    rgbx8888_a8 = 0x38415852,
    bgrx8888_a8 = 0x38415842,

    rgb888_a8 = 0x38413852,
    bgr888_a8 = 0x38413842,
    rgb565_a8 = 0x38413552,
    bgr565_a8 = 0x38413542,

    nv24 = 0x3432564e, // non-subsampled Cr:Cb plane
    nv42 = 0x3234564e, // non-subsampled Cb:Cr plane
    p210 = 0x30313250, // 2x1 subsampled Cr:Cb plane, 10 bit per channel
    p010 = 0x30313050, // 2x2 subsampled Cr:Cb plane 10 bits per channel
    p012 = 0x32313050, // 2x2 subsampled Cr:Cb plane 12 bits per channel
    p016 = 0x36313050, // 2x2 subsampled Cr:Cb plane 16 bits per channel

    axbxgxrx106106106106 = 0x30314241, // [63:0] A:x:B:x:G:x:R:x 10:6:10:6:10:6:10:6 little endian

    nv15 = 0x3531564e, // 2x2 subsampled Cr:Cb plane

    q410 = 0x30313451,
    q401 = 0x31303451,

    xrgb16161616 = 0x38345258, // [63:0] x:R:G:B 16:16:16:16 little endian
    xbgr16161616 = 0x38344258, // [63:0] x:B:G:R 16:16:16:16 little endian
    argb16161616 = 0x38345241, // [63:0] A:R:G:B 16:16:16:16 little endian
    abgr16161616 = 0x38344241, // [63:0] A:B:G:R 16:16:16:16 little endian

    c1 = 0x20203143, // [7:0] C0:C1:C2:C3:C4:C5:C6:C7 1:1:1:1:1:1:1:1 eight pixels/byte
    c2 = 0x20203243, // [7:0] C0:C1:C2:C3 2:2:2:2 four pixels/byte
    c4 = 0x20203443, // [7:0] C0:C1 4:4 two pixels/byte
    d1 = 0x20203144, // [7:0] D0:D1:D2:D3:D4:D5:D6:D7 1:1:1:1:1:1:1:1 eight pixels/byte
    d2 = 0x20203244, // [7:0] D0:D1:D2:D3 2:2:2:2 four pixels/byte
    d4 = 0x20203444, // [7:0] D0:D1 4:4 two pixels/byte
    d8 = 0x20203844, // [7:0] D
    r1 = 0x20203152, // [7:0] R0:R1:R2:R3:R4:R5:R6:R7 1:1:1:1:1:1:1:1 eight pixels/byte
    r2 = 0x20203252, // [7:0] R0:R1:R2:R3 2:2:2:2 four pixels/byte
    r4 = 0x20203452, // [7:0] R0:R1 4:4 two pixels/byte

    r10 = 0x20303152, // [15:0] x:R 6:10 little endian
    r12 = 0x20323152, // [15:0] x:R 4:12 little endian

    avuy8888 = 0x59555641, // [31:0] A:Cr:Cb:Y 8:8:8:8 little endian
    xvuy8888 = 0x59555658, // [31:0] X:Cr:Cb:Y 8:8:8:8 little endian

    p030 = 0x30333050, // 2x2 subsampled Cr:Cb plane 10 bits per channel packed
};

}

#endif // HK_WAYLAND_TYPES_H
