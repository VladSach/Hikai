#ifdef HKWAYLAND

#include "wayland.h"

#include "utility/hkassert.h"
#include "containers/hkvector.h"

// #include "vendor/imgui/imgui_impl_linux.h"

#define HK_WAYLAND_DEBUG
#define HK_WAYLAND_DEBUG_EXTENSIVE

#ifndef HK_WAYLAND_DEBUG
#define LOG_DEBUG(...)
#endif
#ifndef HK_WAYLAND_DEBUG_EXTENSIVE
#define LOG_TRACE(...)
#endif

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <sys/socket.h>

namespace hk::platform {

static wl::WaylandContext ctx;

constexpr u32 color_channels = 4;
constexpr u16 header_size = 8;
constexpr u32 cnt_buffers = 1; // Single buffering

void init ()
{
    LOG_DEBUG("Initializing Wayland Backend");

    ctx.display = wl::display_connect();
    ctx.wl_registry = wl::wl_display_get_registry();

    wl::display_roundtrip();

    // Highly unlikely for user to have more than 2 monitors
    ctx.outputs.reserve(2);

    wl::wl_seat_get_pointer();
    wl::wl_seat_get_keyboard();

    wl::display_roundtrip();

    // First thing to do after starting backend is
    // to gather information about system

    // Allocate display, registry, seat, shm, xdg_wm_base
    // everything that is created from registry is a global object
    // so they all are one per instance
    //
    // everything else is per-window basis

    // LOG_INFO("Found", ctx.supported_formats.size(), "supported formats");
    // for (const auto &format : ctx.supported_formats) {
    //     LOG_INFO("+", to_string(format));
    // }
    //
    // LOG_INFO("Found", ctx.outputs.size(), "monitors");
    // for (const auto &output : ctx.outputs) {
    //     LOG_INFO(""
    //         "Display:", output.name, "Model:", output.model, '\n',
    //         "Vendor:", output.vendor, '\n',
    //         "Description:", output.desc, '\n',
    //         "X:", output.x, "Y:", output.y, '\n',
    //         "Width:", output.physical_width, "mm",
    //         "Height:", output.physical_height, "mm", '\n',
    //         "Subpixel Type:", to_string(output.subpixel), '\n',
    //         "Transform", output.transform, '\n',
    //         "Width:", output.width, "Height:", output.height, '\n',
    //         "Refresh rate:", output.hz, "Hz", '\n',
    //         "Scaling:", output.scale, '\n'
    //     );
    // }
}

void deinit()
{

}

b8 process_messages()
{
    // wl::wire_message<4096> msg;
    //
    // while (peek_message(msg)) {
    //     // if (msg.message == WM_QUIT)
    //     //     return false;
    // }
    //
    // return true;

    return wl::display_roundtrip();
}

b8 create_window(const char *title, u32 width, u32 height)
{
    if (ctx.display == -1) {
        LOG_ERROR("Initialize backend before creating a window");
        return false;
    }

    if (!ctx.wl_compositor || !ctx.wl_shm || !ctx.xdg_wm_base) {
        LOG_ERROR("Wait for Wayland to create all global objets");
        return false;
    }

    LOG_DEBUG("Creating Wayland Window");

    // SOFTWARE RENDERING
    // The stride counts how many bytes a horizontal row takes
    u32 stride = width * color_channels;

    // ctx.shm_pool_size = height * stride * cnt_buffers;
    // wl::create_shm(ctx.shm_pool_size);

    ctx.wl_surface = wl::wl_compositor_create_surface();
    ctx.xdg_surface = wl::xdg_wm_base_get_xdg_surface();
    ctx.xdg_toplevel = wl::xdg_surface_get_toplevel();

    wl::xdg_toplevel_set_title(title);
    wl::xdg_toplevel_set_app_id("Hikai");
    // wl::xdg_toplevel_set_max_size(width, height);

    wl::wl_surface_commit();

    // ctx.wl_shm_pool = wl::wl_shm_create_pool();
    // ctx.wl_buffer = wl::wl_shm_pool_create_buffer();

    wl::display_roundtrip();
    wl::wl_surface_commit();

    return true;
}

namespace wl {

b8 display_roundtrip()
{
    pollfd pfds[1] = {};
    pfds[0].fd = ctx.display;
    pfds[0].events = POLLIN;

    while (poll(pfds, 1, 100) > 0) {
        if (!(pfds[0].revents & POLLIN)) {
            LOG_DEBUG("NOT READ");
        }

        // MSG_DONTWAIT
        // MSG_PEEK
        // MSG_WAITALL

        u8 read_buf[4096] = {};
        i64 read_bytes = recv(ctx.display, read_buf, sizeof(read_buf), 0);
        ALWAYS_ASSERT(read_bytes != -1,
                    "Failed to get Wayland message, errno:", errno);

        wire_message<4096> msg(read_buf, static_cast<u64>(read_bytes));

        while (peek_message(msg)) {
            // if (msg.message == WM_QUIT)
            //     return false;


            // if (ctx.state == WaylandContext::ACKED_CONFIGURE) {
            //     // Render a frame
            //
            //     u32 *pixels = reinterpret_cast<u32*>(ctx.shm_pool_data);
            //     for (u32 i = 0; i < ctx.width * ctx.height; ++i) {
            //         u8 r = 0xff;
            //         u8 g = 0;
            //         u8 b = 0;
            //         pixels[i] = (r << 16) | (g << 8) | b;
            //     }
            //
            //     wl_surface_attach();
            //     wl_surface_commit();
            //
            //     ctx.state = WaylandContext::ATTACHED;
            // }
        }
    }

    return true;
}

i32 display_connect()
{
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    const char *wayland_display = getenv("WAYLAND_DISPLAY");

    // TODO: add fallback instead
    ALWAYS_ASSERT(xdg_runtime_dir, "Failed to locate xdg runtime env");
    ALWAYS_ASSERT(wayland_display, "Failed to locate wayland display env");

    u64 xdg_runtime_dir_len = strlen(xdg_runtime_dir);
    u64 wayland_display_len = strlen(wayland_display);

    u64 socket_path_len = 0;

    sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;

    ALWAYS_ASSERT(xdg_runtime_dir_len <= sizeof(addr.sun_path) - 1,
                  "Wayland socket path is too long");

    // Concat path
    memcpy(addr.sun_path, xdg_runtime_dir, xdg_runtime_dir_len);
    socket_path_len += xdg_runtime_dir_len;

    addr.sun_path[socket_path_len++] = '/';

    memcpy(addr.sun_path + socket_path_len, wayland_display, wayland_display_len);
    socket_path_len += wayland_display_len;

    // Connect to wayland
    i32 fd = socket(AF_UNIX, SOCK_STREAM, 0);
    ALWAYS_ASSERT(fd > 0, "Failed to create wayland socket");

    // Setting a socket to non-blocking
    fcntl(fd, F_SETFL, O_NONBLOCK);

    i32 res = connect(fd, (sockaddr *)&addr, sizeof(addr));
    ALWAYS_ASSERT(!res, "Failed to connect to wayland socket");

    LOG_TRACE("Connected to Wayland Socket at:", addr.sun_path);

    return fd;
}

/* ====== Wayland Requests ====== */

u32 wl_display_get_registry()
{
    constexpr u16 opcode = static_cast<u16>(opcode::wl_display_get_registry);

    create_wayland_object(ctx.wl_display, opcode);

    LOG_TRACE("-> wl_display#", ctx.wl_display,
              ".get_registry: wl_registry=", ctx.curr_id);

    return ctx.curr_id;
}

u32 wl_registry_bind(u32 name, char *interface, u32 interface_len, u32 version)
{
    constexpr u16 opcode = static_cast<u16>(opcode::wl_registry_bind);

    u16 size = header_size + sizeof(name) +
               sizeof(interface_len) + roundup_4(interface_len) +
               sizeof(version) + sizeof(ctx.curr_id);

    ALWAYS_ASSERT(size == roundup_4(size),
                  "Message must by aligned by 32 bits");

    ++ctx.curr_id;

    wire_message<512> msg;
    // Header
    msg.write_u32(ctx.wl_registry);
    msg.write_u16(opcode);
    msg.write_u16(size);
    // Message
    msg.write_u32(name);
    msg.write_string(interface, interface_len);
    msg.write_u32(version);
    msg.write_u32(ctx.curr_id);

    ALWAYS_ASSERT(msg.size() == roundup_4(msg.size()),
                  "Message must by aligned by 32 bits");

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), 0);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> wl_registry#", ctx.wl_registry,
              ".bind: name=", name,
              "interface=", interface,
              "version=", version);

    return ctx.curr_id;
}

u32 wl_compositor_create_surface()
{
    constexpr u16 opcode =
        static_cast<u16>(opcode::wl_compositor_create_surface);

    create_wayland_object(ctx.wl_compositor, opcode);

    LOG_TRACE("-> wl_compositor#", ctx.wl_compositor,
              ".create_surface: wl_surface=", ctx.curr_id);

    return ctx.curr_id;
}

u32 wl_shm_pool_create_buffer()
{
    constexpr u16 opcode = static_cast<u16>(opcode::wl_shm_pool_create_buffer);
    constexpr u16 size = header_size +
                         sizeof(ctx.curr_id) + sizeof(ctx.shm) +
                         sizeof(u32) + sizeof(ctx.width) + sizeof(ctx.height) +
                         sizeof(wl::Format);

    u32 stride = ctx.width * color_channels;

    ++ctx.curr_id;

    wire_message<128> msg;
    msg.write_u32(ctx.wl_shm_pool);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ctx.curr_id);     // buffer to create
    msg.write_u32(0);               // buffer byte offset within the pool
    msg.write_u32(ctx.width);       // buffer width, in pixels
    msg.write_u32(ctx.height);      // buffer height, in pixels
    msg.write_u32(stride);          // stride
    msg.write_u32(static_cast<u32>(wl::Format::xrgb8888)); // format

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> wl_shm_pool#", ctx.wl_shm_pool,
              ".create_buffer: wl_buffer=", ctx.curr_id);

    return ctx.curr_id;
}

u32 wl_shm_create_pool()
{
    ALWAYS_ASSERT(ctx.shm_pool_size > 0, "Linux shm should be allocated first");

    constexpr u16 opcode = static_cast<u16>(opcode::wl_shm_create_pool);
    constexpr u16 size = header_size +
                         sizeof(ctx.curr_id) + sizeof(ctx.shm_pool_size);

    ++ctx.curr_id;

    wire_message<128> msg;
    msg.write_u32(ctx.wl_shm);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ctx.curr_id);
    msg.write_u32(ctx.shm_pool_size); // pool size in bytes

    // Send the file descriptor as ancillary data
    char buf[CMSG_SPACE(sizeof(ctx.shm))] = "";
    iovec io = {.iov_base = msg.buffer(), .iov_len = size};

    msghdr socket_msg = {};
    socket_msg.msg_iov = &io;
    socket_msg.msg_iovlen = 1;
    socket_msg.msg_control = buf;
    socket_msg.msg_controllen = sizeof(buf);

    cmsghdr *cmsg = CMSG_FIRSTHDR(&socket_msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(ctx.shm));

    *((int *)CMSG_DATA(cmsg)) = ctx.shm;
    socket_msg.msg_controllen = CMSG_SPACE(sizeof(ctx.shm));


    i32 err = sendmsg(ctx.display, &socket_msg, 0);
    ALWAYS_ASSERT(err != -1, "Failed to send Wayland message: ", errno);

    LOG_TRACE("-> wl_shm#", ctx.wl_shm, ".create_pool: wl_shm_pool=", ctx.curr_id);

    return ctx.curr_id;
}

void wl_surface_attach()
{
    ALWAYS_ASSERT(ctx.wl_buffer);

    constexpr u16 opcode = static_cast<u16>(opcode::wl_surface_attach);
    constexpr u16 size = header_size + sizeof(ctx.wl_buffer) +
                         sizeof(u32) + sizeof(u32);

    wire_message<128> msg;
    msg.write_u32(ctx.wl_surface);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ctx.wl_buffer);
    msg.write_u32(0);
    msg.write_u32(0);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> wl_surface#", ctx.wl_surface, ".attach");
}

void wl_surface_commit()
{
    constexpr u16 opcode = static_cast<u16>(opcode::wl_surface_commit);

    wire_message<128> msg;
    msg.write_u32(ctx.wl_surface);
    msg.write_u16(opcode);
    msg.write_u16(header_size);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> wl_surface#", ctx.wl_surface, ".commit");
}

void wl_seat_get_pointer()
{
    if (!(ctx.seat.capability & static_cast<u32>(Capability::pointer))) {
        LOG_WARN("No pointer found");
        return;
    }

    if (ctx.wl_pointer != 0) {
        LOG_DEBUG("Pointer already exists");
        return;
    }

    constexpr u16 opcode = static_cast<u16>(opcode::wl_seat_get_pointer);

    wl::create_wayland_object(ctx.wl_seat, opcode);
    ctx.wl_pointer = ctx.curr_id;

    LOG_TRACE("-> wl_seat#", ctx.wl_seat,
            ".get_pointer: wl_pointer=", ctx.wl_pointer);
}

void wl_seat_get_keyboard()
{
    if (!(ctx.seat.capability & static_cast<u32>(Capability::keyboard))) {
        LOG_WARN("No keyboard found");
        return;
    }

    if (ctx.wl_keyboard != 0) {
        LOG_DEBUG("Keyboard already exists");
        return;
    }

    constexpr u16 opcode = static_cast<u16>(opcode::wl_seat_get_keyboard);

    wl::create_wayland_object(ctx.wl_seat, opcode);
    ctx.wl_keyboard = ctx.curr_id;

    LOG_TRACE("-> wl_seat#", ctx.wl_seat,
            ".get_keyboard: wl_keyboard=", ctx.wl_keyboard);
}

u32 xdg_wm_base_get_xdg_surface()
{
    constexpr u16 opcode =
        static_cast<u16>(opcode::xdg_wm_base_get_xdg_surface);
    constexpr u16 size =
        header_size + sizeof(ctx.curr_id) + sizeof(ctx.wl_surface);

    ++ctx.curr_id;

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_wm_base);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ctx.curr_id);
    msg.write_u32(ctx.wl_surface);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_wm_base#", ctx.xdg_wm_base,
              ".get_xdg_surface: xdg_surface=", ctx.curr_id);

    return ctx.curr_id;
}

void xdg_wm_base_pong(u32 ping)
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_wm_base_pong);
    constexpr u16 size = header_size + sizeof(ping);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_wm_base);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ping);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    // LOG_TRACE("-> xdg_wm_base#", ctx.xdg_wm_base, ".pong: ping=", ping);
}

u32 xdg_surface_get_toplevel()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_surface_get_toplevel);

    create_wayland_object(ctx.xdg_surface, opcode);

    LOG_TRACE("-> xdg_surface#", ctx.xdg_surface,
              ".get_toplevel: xdg_toplevel=", ctx.curr_id);

    return ctx.curr_id;
}

void xdg_surface_ack_configure(u32 configure)
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_surface_ack_configure);
    constexpr u16 size = header_size + sizeof(configure);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_surface);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(configure);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_surface#", ctx.xdg_surface, ".ack_configure: configure=", configure);
}

void xdg_toplevel_set_title(const char *title)
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_title);

    u32 title_len = strlen(title) + 1;
    u16 size = header_size + sizeof(title_len) + roundup_4(title_len);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_string(title, title_len);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".set_title: title=", title);
}

void xdg_toplevel_set_app_id(const char *id)
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_app_id);

    u32 id_len = strlen(id) + 1;
    u16 size = header_size + sizeof(id_len) + roundup_4(id_len);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_string(id, id_len);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".set_app_id: id=", id);
}

void xdg_toplevel_set_max_size(u32 width, u32 height)
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_max_size);
    constexpr u16 size = header_size + sizeof(width) + sizeof(height);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(width);
    msg.write_u32(height);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel,
              ".set_max_size: width=", width, "height=", height);
}

void xdg_toplevel_set_maximized()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_maximized);
    constexpr u16 size = header_size;

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".set_maximized");
}

void xdg_toplevel_unset_maximized()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_unset_maximized);
    constexpr u16 size = header_size;

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".unset_maximized");
}

void xdg_toplevel_set_fullscreen()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_fullscreen);
    constexpr u16 size = header_size + sizeof(ctx.wl_output);

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);
    msg.write_u32(ctx.wl_output);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel,
              ".set_fullscreen: wl_output=", ctx.wl_output);
}

void xdg_toplevel_unset_fullscreen()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_unset_fullscreen);
    constexpr u16 size = header_size;

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".set_unfullscreen");
}

void xdg_toplevel_set_minimized()
{
    constexpr u16 opcode = static_cast<u16>(opcode::xdg_toplevel_set_minimized);
    constexpr u16 size = header_size;

    wire_message<128> msg;
    msg.write_u32(ctx.xdg_toplevel);
    msg.write_u16(opcode);
    msg.write_u16(size);

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);

    LOG_TRACE("-> xdg_toplevel#", ctx.xdg_toplevel, ".set_minimized");
}

/* ====== Wayland Events ====== */

void wl_display_error(wire_message<4096> &msg)
{
    char error[512] = "";

    u32 object_id = msg.read_u32();
    u32 code = msg.read_u32();

    u32 error_len = msg.read_u32();
    msg.read_blob(error, roundup_4(error_len));

    LOG_FATAL("Wayland Error - ObjectID:", object_id,
              "code:", code, "error:", error);

    // TODO: exit?
    ALWAYS_ASSERT(0, "You've met with a terrible fate, haven't you?");
}

void wl_registry_global(wire_message<4096> &msg)
{
    u32 name = msg.read_u32();
    u32 interface_length = msg.read_u32();
    char interface[512] = "";

    ALWAYS_ASSERT(roundup_4(interface_length) <= (sizeof(interface) - 1));

    msg.read_blob(interface, roundup_4(interface_length));

    // The length includes the NULL terminator
    ALWAYS_ASSERT(interface[interface_length - 1] == 0);

    u32 version = msg.read_u32();

    // LOG_TRACE("<- wl_registry#", ctx.wl_registry,
    //           ".global: name=", name,
    //           "interface=", interface,
    //           "version=", version);

    // ALWAYS_ASSERT(size ==
    //               sizeof(u32) + sizeof(u32) + sizeof(u16) + sizeof(name) +
    //               sizeof(interface_length) + roundup_4(interface_length) +
    //               sizeof(version));

    constexpr char const *wl_shm_interface = "wl_shm";
    constexpr char const *wl_seat_interface = "wl_seat";
    constexpr char const *wl_output_interface = "wl_output";
    constexpr char const *xdg_wm_base_interface = "xdg_wm_base";
    constexpr char const *wl_compositor_interface = "wl_compositor";

    if (strcmp(interface, wl_seat_interface) == 0) {
        ctx.wl_seat = wl_registry_bind(name, interface, interface_length, version);
    } else if (strcmp(interface, wl_compositor_interface) == 0) {
        ctx.wl_compositor = wl_registry_bind(name, interface, interface_length, version);
    } else if (strcmp(interface, wl_shm_interface) == 0) {
        ctx.wl_shm = wl_registry_bind(name, interface, interface_length, version);
    } else if (strcmp(interface, wl_output_interface) == 0) {
        ctx.wl_output = wl_registry_bind(name, interface, interface_length, version);
    } else if (strcmp(interface, xdg_wm_base_interface) == 0) {
        ctx.xdg_wm_base = wl_registry_bind(name, interface, interface_length, version);
    } else {
        // LOG_TRACE("Unused interface:", interface);
    }
}

b8 peek_message(wire_message <4096> &msg)
{
    if (msg.size() <= 0) { return false; }

    ALWAYS_ASSERT(msg.size() >= 8, "Message can't be less then a header size");

    u32 object_id = msg.read_u32();
    ALWAYS_ASSERT(object_id <= ctx.curr_id, "Wayland ObjectID is wrong");

    u16 opcode = msg.read_u16();

    u16 size = msg.read_u16();
    ALWAYS_ASSERT(roundup_4(size) <= size);

    ALWAYS_ASSERT(size <= header_size + msg.size(), " ");

    // Display events
    if (object_id == ctx.wl_display) {
        if (opcode == static_cast<u16>(opcode::wl_display_error)) {
            wl_display_error(msg);
        }
    }

    // Registry events
    else if (object_id == ctx.wl_registry) {
        if (opcode == static_cast<u16>(opcode::wl_registry_global)) {
            wl_registry_global(msg);
        }
    }

    // Shm events
    else if (object_id == ctx.wl_shm) {
        if (opcode == static_cast<u16>(opcode::wl_shm_format)) {
            u32 format = msg.read_u32();
            ctx.supported_formats.push_back(static_cast<wl::Format>(format));

            // LOG_TRACE("<- wl_shm#", ctx.wl_shm, ".format: format=", format);
        }
    }

    // Buffer events
    else if (object_id == ctx.wl_buffer) {
        if (opcode == static_cast<u16>(opcode::wl_buffer_release)) {
            LOG_TRACE("<- wl_buffer#", ctx.wl_buffer, ".release");
        }
    }

    // Surface events
    // else if (object_id == ctx.wl_surface) {
    //     if (opcode == static_cast<u16>(opcode::wl_surface_enter)) {
    //         u32 object_id = msg.read_u32();
    //         LOG_TRACE("<- wl_surface#", ctx.wl_surface,
    //                   ".enter: output=", object_id);
    //     }
    //     if (opcode == static_cast<u16>(opcode::wl_surface_leave)) {
    //         u32 object_id = msg.read_u32();
    //         LOG_TRACE("<- wl_surface#", ctx.wl_surface,
    //                   ".leave: output=", object_id);
    //     }
    // }

    // Seat events
    else if (object_id == ctx.wl_seat) {
        if (opcode == static_cast<u16>(opcode::wl_seat_capabilities)) {
            ctx.seat.capability = msg.read_u32();
            LOG_TRACE("<- wl_seat#", ctx.wl_seat,
                      ".capabilities: capability=", ctx.seat.capability);
        }

        if (opcode == static_cast<u16>(opcode::wl_seat_name)) {
            char name[128] = "";
            u32 name_length = msg.read_u32();
            msg.read_blob(name, roundup_4(name_length));
            ctx.seat.name = name;
            // LOG_TRACE("<- wl_seat#", ctx.wl_seat, ".name: name=", name);
            // TODO: what do I do with it next?
        }
    }

    /* === Input events === */
    // Pointer events
    // else if (object_id == ctx.wl_pointer) {
    //
    // }

    // Keyboard events
    // else if (object_id == ctx.wl_keyboard) {
    //
    // }

    // Output events
    else if (object_id == ctx.wl_output) {
        static WaylandContext::WlOutput output = {};

        if (opcode == static_cast<u16>(opcode::wl_output_geometry)) {
            output.x = msg.read_u32();
            output.y = msg.read_u32();
            output.physical_width = msg.read_u32();
            output.physical_height = msg.read_u32();
            output.subpixel = static_cast<wl::Subpixel>(msg.read_u32());

            char make[128] = "";
            u32 make_length = msg.read_u32();
            msg.read_blob(make, roundup_4(make_length));
            output.vendor = make;

            char model[128] = "";
            u32 model_length = msg.read_u32();
            msg.read_blob(model, roundup_4(model_length));
            output.model = model;

            output.transform = msg.read_u32();

            // LOG_TRACE("<- wl_output#", ctx.wl_output, ".geometry:\n",
            //           "\t" "x = ", x, "\n"
            //           "\t" "y = ", y, "\n"
            //           "\t" "physical_width = ", physical_width, "\n"
            //           "\t" "physical_height = ", physical_height, "\n"
            //           "\t" "subpixel = ", to_string(subpixel), "\n"
            //           "\t" "make = ", make, "\n"
            //           "\t" "model = ", model, "\n"
            //           "\t" "transform = ", transform);
        }

        if (opcode == static_cast<u16>(opcode::wl_output_mode)) {
            // Non-current modes are deprecated
            msg.read_u32(); // mode

            output.width = msg.read_u32();
            output.height = msg.read_u32();

            // Wayland gives refresh rate in mHz,
            // that is refresh rate of 60000 is 60Hz.
            output.hz = msg.read_u32() * .001f;

            // LOG_TRACE("<- wl_output#", ctx.wl_output, ".mode:\n",
            //           "\t" "mode = ", mode, "\n"
            //           "\t" "width = ", width, "\n"
            //           "\t" "height = ", height, "\n"
            //           "\t" "refresh = ", refresh);
        }


        if (opcode == static_cast<u16>(opcode::wl_output_done)) {
            ctx.outputs.push_back(output);
            output = {};

            LOG_TRACE("<- wl_output#", ctx.wl_output, ".done");
        }

        if (opcode == static_cast<u16>(opcode::wl_output_scale)) {
            // Wayland supports only integer scale values,
            // so this value is not really truthful
            // this can be solved though via wp_fractional_scale_manager

            output.scale = static_cast<f32>(msg.read_u32());

            // LOG_TRACE("<- wl_output#", ctx.wl_output, ".scale: factor =", factor);
        }

        if (opcode == static_cast<u16>(opcode::wl_output_name)) {
            char name[128] = "";
            u32 name_length = msg.read_u32();
            msg.read_blob(name, roundup_4(name_length));
            output.name = name;

            // LOG_TRACE("<- wl_output#", ctx.wl_output, ".name: name =", name);
        }

        if (opcode == static_cast<u16>(opcode::wl_output_description)) {
            char desc[128] = "";
            u32 desc_length = msg.read_u32();
            msg.read_blob(desc, roundup_4(desc_length));
            output.desc = desc;

            // LOG_TRACE("<- wl_output#", ctx.wl_output,
            //           ".description: description =", desc);
        }
    }

    // XDG wm_base events
    else if (object_id == ctx.xdg_wm_base) {
        if (opcode == static_cast<u16>(opcode::xdg_wm_base_ping)) {
            u32 ping = msg.read_u32();
            // LOG_TRACE("<- xdg_wm_base#", ctx.xdg_wm_base, ".ping: ping=", ping);
            xdg_wm_base_pong(ping);
        }
    }

    // XDG surface events
    else if (object_id == ctx.xdg_surface) {
        if (opcode == static_cast<u16>(opcode::xdg_surface_configure)) {
            u32 configure = msg.read_u32();
            LOG_TRACE("<- xdg_surface#", ctx.xdg_surface,
                      ".configure: configure=", configure);
            xdg_surface_ack_configure(configure);
            ctx.state = WaylandContext::ACKED_CONFIGURE;
        }
    }

    // XDG toplevel events
    else if (object_id == ctx.xdg_toplevel) {
        if (opcode == static_cast<u16>(opcode::xdg_toplevel_configure)) {
            u32 width = msg.read_u32();
            u32 height = msg.read_u32();

            char states[128] = "";
            u32 states_length = msg.read_u32();
            msg.read_blob(states, roundup_4(states_length));

            LOG_TRACE("<- xdg_toplevel#", ctx.xdg_toplevel,
                      ".configure: width=", width, "height=", height,
                      "states=", states);
        }

        if (opcode == static_cast<u16>(opcode::xdg_toplevel_close)) {
            LOG_TRACE("<- xdg_toplevel#", ctx.xdg_toplevel, ".close");
        }

        if (opcode == static_cast<u16>(opcode::xdg_toplevel_wm_capabilities)) {
            char capabilities[128] = "";
            u32 capabilities_length = msg.read_u32();
            msg.read_blob(capabilities, roundup_4(capabilities_length));

            LOG_TRACE("<- xdg_toplevel#", ctx.xdg_toplevel,
                      ".wm_capabilities: capabilities=", capabilities);
        }
    }

    // Ignore the message
    else {
        char bin[512] = "";
        msg.read_blob(bin, roundup_4(size - header_size));
        LOG_TRACE("Ignored - ObjectID:", object_id,
                  "opcode:", opcode, "message length:", size);
    }

    return true;
}

void create_wayland_object(u32 object_id, u16 opcode)
{
    constexpr u16 size = header_size + sizeof(ctx.curr_id);

    ++ctx.curr_id;

    wire_message<128> msg;
    msg.write_u32(object_id);   // write object id
    msg.write_u16(opcode);      // write opcode
    msg.write_u16(size);        // write size
    msg.write_u32(ctx.curr_id); // write arguments

    i32 bytes = send(ctx.display, msg.buffer(), msg.size(), MSG_DONTWAIT);
    ALWAYS_ASSERT(static_cast<i32>(msg.size()) == bytes);
}

void create_shm(u64 size)
{
    // Could also use mkstemp or shm_open
    // But it's not like Hikai is meant to be run on something other then Linux
    i32 fd = memfd_create("hk-shm", 0);
    ALWAYS_ASSERT(fd != -1);

    // Resize shm
    i32 err = ftruncate(fd, size);
    ALWAYS_ASSERT(err != -1);

    ctx.shm = fd;

    // Map it
    ctx.shm_pool_data =
        mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ALWAYS_ASSERT(ctx.shm_pool_data != MAP_FAILED, "Failed to map shm");

    LOG_DEBUG("Created shared memory file");
}

} // namespace wl

} // namespace hk::platform

#endif // HKWAYLAND
