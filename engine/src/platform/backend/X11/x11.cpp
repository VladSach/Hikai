#include "x11.h"

#include "x11context.h"

#include "utility/hkassert.h"
#include "containers/hkvector.h"

#include "core/events.h"

namespace hk::platform {

static x11::X11Context ctx;

u32 window_width() { return ctx.win_width; }
u32 window_height() { return ctx.win_height; }

void init()
{
    LOG_DEBUG("Initializing X11 Backend");

    ctx.display = XOpenDisplay(NULL);
    ALWAYS_ASSERT(ctx.display, "Failed to open x11 display");

    XSetErrorHandler(x11::handle_error);

    LOG_TRACE("Server Vendor:", XServerVendor(ctx.display));
    LOG_TRACE("File descriptor:", XConnectionNumber(ctx.display));
    LOG_TRACE("Protocol Version:", XProtocolVersion(ctx.display));

    i32 op, e, err;
    ctx.is_xwayland = XQueryExtension(ctx.display, "XWAYLAND", &op, &e, &err);
    LOG_TRACE("Server:", ctx.is_xwayland ? "XWayland" : "Native");

    ctx.wm_delete = XInternAtom(ctx.display, "WM_DELETE_WINDOW", False);
    ctx.root = XDefaultRootWindow(ctx.display);

    // ctx.xwayland_scale =
    //     XInternAtom(ctx.display, "_XWAYLAND_GLOBAL_OUTPUT_SCALE", False);

    query_monitor_info();

    /* FULLSCREEN
    Atom wm_state   = XInternAtom(ctx.display, "_NET_WM_STATE", true );
    Atom wm_fullscreen = XInternAtom(ctx.display, "_NET_WM_STATE_FULLSCREEN", true );

    XChangeProperty(ctx.display, ctx.window, wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&wm_fullscreen, 1);
    */

    /* TODO:
     *
     * get monitor information
     * get keyboard, mouse, other pereferei info
     *
     * DPI?
     * fullscreen
     *
     * cursor operations
     * raw mouse input
     * minimize maximize restored
     * mouse wheel input
     */

    // Then decouple window from platform
}

void deinit()
{
    XDestroyWindow(ctx.display, ctx.window);
    XCloseDisplay(ctx.display);
}

b8 process_messages()
{
    XEvent event = {};

    // Non-blocking events
    while(XPending(ctx.display)) {
        XNextEvent(ctx.display, &event);

        if (event.type == ClientMessage) {
            if (event.xclient.data.l[0] == ctx.wm_delete) {
                LOG_DEBUG("You Should Delete Yourself... Now!");
                return false;
            }
        }

        x11::parse_event(event);
    }

    return true;
}

b8 create_window(const char *title, u32 width, u32 height)
{
    if (!ctx.display || !ctx.root) {
        LOG_ERROR("Initialize backend before creating a window");
        return false;
    }

    LOG_DEBUG("Creating X11 Window \"", title, "\"");

    ctx.win_width = width;
    ctx.win_height = height;

    constexpr u32 background = 0x009DCEFF; // #9DCEFF
    constexpr u32 event_mask =
                        KeyPressMask    | KeyReleaseMask    |
                        ButtonPressMask | ButtonReleaseMask |
                        EnterWindowMask | LeaveWindowMask   |
                        PointerMotionMask |
                        ExposureMask | StructureNotifyMask |
                        NoEventMask;

    XSetWindowAttributes attributes = {};
    attributes.background_pixel = background;
    attributes.event_mask = event_mask;

    constexpr i32 attributes_mask = CWBackPixel | CWEventMask;

    ctx.window = XCreateWindow(
        ctx.display,     // display
        ctx.root,        // parent
        0, 0,            // x, y position
        width, height,   // width, height
        0,               // border width
        CopyFromParent,  // window depth
        CopyFromParent,  // window class
        CopyFromParent,  // window visual
        attributes_mask, // value set in attributes
        &attributes);    // window options

    XSetWMProtocols(ctx.display, ctx.window, &ctx.wm_delete, 1);

    XStoreName(ctx.display, ctx.window, title);

    // Show Window
    XMapWindow(ctx.display, ctx.window);

    return true;
}

void query_monitor_info()
{
    XRROutputInfo *info;
    XRRCrtcInfo *crtc_info;

    XRRScreenResources *screen;
    screen = XRRGetScreenResources(ctx.display, ctx.root);

    u32 output_count = screen->noutput;
    for (u32 i = 0; i < output_count; ++i) {
        info = XRRGetOutputInfo(ctx.display, screen, screen->outputs[i]);
        if (info->connection == RR_Connected) {
            for (u32 icrtc = info->ncrtc; icrtc > 0; --icrtc) {
                crtc_info = XRRGetCrtcInfo(ctx.display, screen, info->crtc);

                f32 active_rate = 0;
                for (int i = 0; i < screen->nmode; ++i) {
                    XRRModeInfo mode_info = screen->modes[i];
                    if (mode_info.id != crtc_info->mode) { continue; }

                    active_rate =
                        static_cast<f32>(mode_info.dotClock) /
                        (mode_info.hTotal * mode_info.vTotal);
                }

                LOG_INFO("Name:", info->name);
                LOG_INFO("Position:", crtc_info->x, 'x', crtc_info->y,
                         /* If scaling applied dimenions are incorrect */
                         "Dimensions:", crtc_info->width, 'x', crtc_info->height);
                LOG_INFO("Subpixel Type:", info->subpixel_order,
                         "Refresh rate:", active_rate, "Hz");
                LOG_INFO("Width:", info->mm_width, "mm",
                         "Height:", info->mm_height, "mm");

                /*
                * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
                *
                *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
                *         = N pixels / (M inch / 25.4)
                *         = N * 25.4 pixels / M inch
                */
                // Also wrong
                u32 xres = ((((double) crtc_info->width) * 25.4) /
                    ((double) info->mm_width)) + (0.5 * 25.4);
                u32 yres = ((((double) crtc_info->height) * 25.4) /
                    ((double) info->mm_height)) + 0.5;

                // FIX: missing:
                // LOG_INFO(""
                //     "Display:", output.name, "Model:", output.model, '\n',
                //     "Vendor:", output.vendor, '\n',
                //     "Scaling:", output.scale, '\n'
                // );

                XRRFreeCrtcInfo(crtc_info);
            }
        }
        XRRFreeOutputInfo(info);
    }
    XRRFreeScreenResources(screen);

    // Try to get scaling from .Xresources
    char *resource = XResourceManagerString(ctx.display);
    if (!resource) {
        return;
    }

    XrmInitialize();
    XrmDatabase db = XrmGetStringDatabase(resource);

    XrmValue value;
    char *type = NULL;
    if (XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True) {
        if (value.addr) {
            f32 dpi = atof(value.addr);
        }
    }
}

namespace x11 {

// FIX: temp
Display* get_display() { return ctx.display; }
Window get_window() { return ctx.window; }

void parse_event(XEvent event)
{
    switch(event.type) {
    case KeyPress:
    case KeyRelease: {
        b8 is_pressed = event.type == KeyPress;

        // TEST: I'm unsure whether I care about shift modifier
        // b8 is_shifted = event.xkey.state & ShiftMask;

        KeySym keysym = XkbKeycodeToKeysym(ctx.display,
                                           event.xkey.keycode, 0, 0);

        hk::event::EventContext context;
        context.u16[0] = map_keysym(keysym);
        context.u16[1] = static_cast<u16>(is_pressed);
        hk::event::fire(is_pressed ?
                        hk::event::EVENT_KEY_PRESSED :
                        hk::event::EVENT_KEY_RELEASED,
                        context);
    } break;

    case ButtonPress:
    case ButtonRelease: {
        b8 is_pressed = event.type == ButtonPress;

        input::Button button = input::Button::EMPTY_KEY;

        switch(event.xbutton.button) {
        case Button1: { button = input::Button::BUTTON_LEFT;   } break;
        case Button2: { button = input::Button::BUTTON_MIDDLE; } break;
        case Button3: { button = input::Button::BUTTON_RIGHT;  } break;
        default: {
            LOG_WARN("Unrecognized Mouse Button:", event.xbutton.button);
        }
        }

        hk::event::EventContext context;
        context.u16[0] = static_cast<u16>(button);
        context.u16[1] = static_cast<u16>(is_pressed);
        hk::event::fire(is_pressed ?
                        hk::event::EVENT_MOUSE_PRESSED :
                        hk::event::EVENT_MOUSE_RELEASED,
                        context);

    } break;

    case MotionNotify: {
        hk::event::EventContext context;
        context.i32[0] = event.xmotion.x;
        context.i32[1] = event.xmotion.y;
        hk::event::fire( hk::event::EVENT_MOUSE_MOVED, context);
    } break;

    // Resize event
    case ConfigureNotify: {
        // event.xconfigure.window;

        u32 width = event.xconfigure.width;
        u32 height = event.xconfigure.height;

        // if (width_ == width && height_ == height) {
        //     LOG_TRACE("Window moved");
        //     break;
        // }

        // width_ = width;
        // height_ = height;

        hk::event::EventContext context;
        context.u32[0] = width;
        context.u32[1] = height;
        hk::event::fire(hk::event::EVENT_WINDOW_RESIZE, context);

        // LOG_INFO("Window's size set to:", width, "x", height);
    } break;

    default: break;
    }
}

i32 handle_error(Display *display, XErrorEvent *event)
{
    char desc[512] = {};
    XGetErrorText(display, event->error_code, desc, 512);

    LOG_ERROR("X11 Error:", event->error_code, desc);

    return 0;
}

input::Button map_keysym(KeySym keysym)
{
    using namespace input;

    switch(keysym) {

    case XK_BackSpace: { return Button::KEY_BACKSPACE; }
    case XK_Tab:       { return Button::KEY_TAB; }
    case XK_Return:    { return Button::KEY_ENTER; }

    // Button::KEY_SHIFT
    // Button::KEY_CTRL
    // Button::KEY_ALT
    case XK_Pause:     { return Button::KEY_PAUSE; }
    case XK_Caps_Lock: { return Button::KEY_CAPITAL; }

    case XK_Escape:      { return Button::KEY_ESC; }
    case XK_Mode_switch: { return Button::KEY_MODECHANGE; }
    case XK_space:       { return Button::KEY_SPACE; }

    case XK_Prior:   { return Button::KEY_PAGEUP; }
    case XK_Next:    { return Button::KEY_PAGEDOWN; }
    case XK_End:     { return Button::KEY_END; }
    case XK_Home:    { return Button::KEY_HOME; }
    case XK_Left:    { return Button::KEY_LEFT; }
    case XK_Up:      { return Button::KEY_UP; }
    case XK_Right:   { return Button::KEY_RIGHT; }
    case XK_Down:    { return Button::KEY_DOWN; }
    case XK_Select:  { return Button::KEY_SELECT; }
    case XK_Print:   { return Button::KEY_PRINT; }
    case XK_Execute: { return Button::KEY_EXECUTE; }
    // case XK_SYSRQ:   { return Button::KEY_SNAPSHOT; }
    case XK_Insert:  { return Button::KEY_INSERT; }
    case XK_Delete:  { return Button::KEY_DELETE; }
    case XK_Help:    { return Button::KEY_HELP; }

    case XK_0: { return Button::KEY_0; }
    case XK_1: { return Button::KEY_1; }
    case XK_2: { return Button::KEY_2; }
    case XK_3: { return Button::KEY_3; }
    case XK_4: { return Button::KEY_4; }
    case XK_5: { return Button::KEY_5; }
    case XK_6: { return Button::KEY_6; }
    case XK_7: { return Button::KEY_7; }
    case XK_8: { return Button::KEY_8; }
    case XK_9: { return Button::KEY_9; }

    case XK_a: case XK_A: { return Button::KEY_A; }
    case XK_b: case XK_B: { return Button::KEY_B; }
    case XK_c: case XK_C: { return Button::KEY_C; }
    case XK_d: case XK_D: { return Button::KEY_D; }
    case XK_e: case XK_E: { return Button::KEY_E; }
    case XK_f: case XK_F: { return Button::KEY_F; }
    case XK_g: case XK_G: { return Button::KEY_G; }
    case XK_h: case XK_H: { return Button::KEY_H; }
    case XK_i: case XK_I: { return Button::KEY_I; }
    case XK_j: case XK_J: { return Button::KEY_J; }
    case XK_k: case XK_K: { return Button::KEY_K; }
    case XK_l: case XK_L: { return Button::KEY_L; }
    case XK_m: case XK_M: { return Button::KEY_M; }
    case XK_n: case XK_N: { return Button::KEY_N; }
    case XK_o: case XK_O: { return Button::KEY_O; }
    case XK_p: case XK_P: { return Button::KEY_P; }
    case XK_q: case XK_Q: { return Button::KEY_Q; }
    case XK_r: case XK_R: { return Button::KEY_R; }
    case XK_s: case XK_S: { return Button::KEY_S; }
    case XK_t: case XK_T: { return Button::KEY_T; }
    case XK_u: case XK_U: { return Button::KEY_U; }
    case XK_v: case XK_V: { return Button::KEY_V; }
    case XK_w: case XK_W: { return Button::KEY_W; }
    case XK_x: case XK_X: { return Button::KEY_X; }
    case XK_y: case XK_Y: { return Button::KEY_Y; }
    case XK_z: case XK_Z: { return Button::KEY_Z; }

    case XK_Meta_L: case XK_Super_L: { return Button::KEY_LSUPER; }
    case XK_Meta_R: case XK_Super_R: { return Button::KEY_RSUPER; }
    // Button::KEY_APPS

    // Button::KEY_SLEEP

    case XK_KP_0: { return Button::KEY_NUMPAD0; }
    case XK_KP_1: { return Button::KEY_NUMPAD1; }
    case XK_KP_2: { return Button::KEY_NUMPAD2; }
    case XK_KP_3: { return Button::KEY_NUMPAD3; }
    case XK_KP_4: { return Button::KEY_NUMPAD4; }
    case XK_KP_5: { return Button::KEY_NUMPAD5; }
    case XK_KP_6: { return Button::KEY_NUMPAD6; }
    case XK_KP_7: { return Button::KEY_NUMPAD7; }
    case XK_KP_8: { return Button::KEY_NUMPAD8; }
    case XK_KP_9: { return Button::KEY_NUMPAD9; }

    case XK_multiply:     { return Button::KEY_MULTIPLY; }
    case XK_KP_Add:       { return Button::KEY_ADD; }
    case XK_KP_Separator: { return Button::KEY_SEPARATOR; }
    case XK_KP_Subtract:  { return Button::KEY_SUBTRACT; }
    case XK_KP_Decimal:   { return Button::KEY_DECIMAL; }
    case XK_KP_Divide:    { return Button::KEY_DIVIDE; }

    case XK_F1:  { return Button::KEY_F1; }
    case XK_F2:  { return Button::KEY_F2; }
    case XK_F3:  { return Button::KEY_F3; }
    case XK_F4:  { return Button::KEY_F4; }
    case XK_F5:  { return Button::KEY_F5; }
    case XK_F6:  { return Button::KEY_F6; }
    case XK_F7:  { return Button::KEY_F7; }
    case XK_F8:  { return Button::KEY_F8; }
    case XK_F9:  { return Button::KEY_F9; }
    case XK_F10: { return Button::KEY_F10; }
    case XK_F11: { return Button::KEY_F11; }
    case XK_F12: { return Button::KEY_F12; }
    case XK_F13: { return Button::KEY_F13; }
    case XK_F14: { return Button::KEY_F14; }
    case XK_F15: { return Button::KEY_F15; }
    case XK_F16: { return Button::KEY_F16; }
    case XK_F17: { return Button::KEY_F17; }
    case XK_F18: { return Button::KEY_F18; }
    case XK_F19: { return Button::KEY_F19; }
    case XK_F20: { return Button::KEY_F20; }
    case XK_F21: { return Button::KEY_F21; }
    case XK_F22: { return Button::KEY_F22; }
    case XK_F23: { return Button::KEY_F23; }
    case XK_F24: { return Button::KEY_F24; }

    case XK_Num_Lock:    { return Button::KEY_NUMLOCK; }
    case XK_Scroll_Lock: { return Button::KEY_SCROLL; }

    case XK_Shift_L:   { return Button::KEY_LSHIFT; }
    case XK_Shift_R:   { return Button::KEY_RSHIFT; }
    case XK_Control_L: { return Button::KEY_LCTRL; }
    case XK_Control_R: { return Button::KEY_RCTRL; }
    case XK_Alt_L:     { return Button::KEY_LALT; }
    case XK_Alt_R:     { return Button::KEY_RALT; }

    case XK_semicolon: { return Button::KEY_SEMICOLON; }
    case XK_equal:     { return Button::KEY_EQUAL; }
    case XK_comma:     { return Button::KEY_COMMA; }
    case XK_minus:     { return Button::KEY_MINUS; }
    case XK_period:    { return Button::KEY_PERIOD; }
    case XK_slash:     { return Button::KEY_SLASH; }
    case XK_grave:     { return Button::KEY_GRAVE; }

    case XK_bracketleft:  case XK_braceleft:  { return Button::KEY_LBRACKET; }
    case XK_backslash:    case XK_bar:        { return Button::KEY_BACKSLASH; }
    case XK_bracketright: case XK_braceright: { return Button::KEY_RBRACKET; }

    case XK_apostrophe: { return Button::KEY_QUOTE; }

    default: {
        LOG_WARN("Unrecognized Keyboard Key:", keysym);
        return Button::EMPTY_KEY;
    }
    }
}

} // namespace x11

} // namespace hk::platform
