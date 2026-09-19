#include "platform/platform.h"
#ifdef HKWIN32

#include "win32.h"
#include "win32context.h"
#include "win32utils.h"

#include "platform/entry/args.h"

#include "utility/hkassert.h"
#include "strings/hklocale.h"

#include "core/events.h"

namespace hk::platform {

static win32::Context ctx;
static SystemSpec sys_specs;

static constexpr char const *wc_name = "Hikai Window Class";

void init()
{
    LOG_DEBUG("Initializing Windows Backend");

    /* TODO: change based on win version + move to per monitor aware
     * SetProcessDPIAware() - Windows Vista and up, only for system aware (?)
     * SetProcessDpiAwareness() - Windows 8.1 and up
     * SetProcessDpiAwarenessContext() - Windows 10, version 1607 and up */
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    ctx.inst = hk::platform::args::hInstance;
    sys_specs.type = SystemSpec::SystemType::WINDOWS;

    query_monitor_info();

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc; // shared across all windows
    wc.hInstance = ctx.inst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = wc_name;

    ALWAYS_ASSERT(RegisterClassEx(&wc), "Failed to register the window class");
}

void deinit()
{
    UnregisterClassA(wc_name, ctx.inst);
}

b8 process_messages()
{
    MSG msg = {};

    // Non-blocking events
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            LOG_DEBUG("You Should Delete Yourself... Now!");
            return false;
        }

        // translate keystroke messages into the right format
        TranslateMessage(&msg);

        // send the message to the WindowProc function
        DispatchMessage(&msg);
    }

    return true;
}

WindowHandle create_window(WindowDesc desc)
{
    // if (!ctx.wc) {
    //     LOG_ERROR("Initialize backend before creating a window");
    //     return false;
    // }

    LOG_DEBUG("Creating Windows Window \"", desc.title, "\"");

    // Allocates the object before CreateWindowEx so that
    // WM_NCCREATE can store a pointer to it via GWLP_USERDATA
    win32::Window *window = &ctx.windows.emplace_back();

    // MonitorSpec monitor = sys_specs.monitors.at(0);
    // if (width_ > info.width) {
    //     width_ = info.width;
    //     LOG_WARN("Window width can not be bigger then screen width:",
    //              width, "vs", info.width);
    // }
    // if (height_ > info.height) {
    //     height_ = info.height;
    //     LOG_WARN("Window height can not be bigger then screen height:",
    //              height, "vs", info.height);
    // }
    //
    // Screen size minus taskbar
    // RECT workArea;
    // SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    // u32 workareaWidth = workArea.right - workArea.left;
    // u32 workareaHeight = workArea.bottom - workArea.top;
    //
    // if (is_fullscreen_) {
    //
    //     // TODO: add Fullscreen
    // }
    //
    // // set the size, but not the position
    // RECT wr = {
    //     0, 0,
    //     static_cast<LONG>(width_),
    //     static_cast<LONG>(height_)
    // };
    // AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    //
    // (workareaWidth  - wr.right)  / 2, // x-position of the window
    // (workareaHeight - wr.bottom) / 2, // y-position of the window
    // wr.right - wr.left,    // width of the window
    // wr.bottom - wr.top,    // height of the window

    constexpr DWORD styles_mask = WS_OVERLAPPEDWINDOW;

    HWND hwnd = CreateWindowExA(
        NULL,
        wc_name,
        desc.title.c_str(),      // name
        styles_mask,
        desc.x, desc.y,          // x, y position
        desc.width, desc.height, // width, height
        NULL, // parent
        NULL,
        ctx.inst, window
    );

    ALWAYS_ASSERT(hwnd, "Failed to create a Window:",
                  win32::get_error_msg(GetLastError()));

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);

    return { static_cast<u8>(ctx.windows.size() - 1), 0 };
}

void show_cursor()
{
    ctx.is_cursor_enabled = true;
    while(ShowCursor(TRUE) < 0);
}

void hide_cursor()
{
    ctx.is_cursor_enabled = false;
    while(ShowCursor(FALSE) >= 0);
}

void lock_cursor()
{
    // RECT r;
    // GetWindowRect(ctx.windows.at(handle.index).hwnd, &r);

    // RECT r = {
    //     x, y,
    //     static_cast<LONG>(width),
    //     static_cast<LONG>(height)
    // };
    // ClipCursor(&r);
}

void unlock_cursor()
{
    ClipCursor(nullptr);
}

void enable_raw_mouse_input()
{
    if (ctx.is_raw_mouse_input_enabled) { return; }

    ctx.is_raw_mouse_input_enabled = true;

    BOOL result;

    RAWINPUTDEVICE rid[1] = {};
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    // RIDEV_INPUTSINK - Enables the caller to receive the input
    // even when the caller is not in the foreground
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = NULL; // if NULL, events follow the keyboard focus
    result = RegisterRawInputDevices(rid, 1, sizeof(rid[0]));

    if (result == FALSE) {
        LOG_WARN("Failed to register raw mouse input");
        LOG_TRACE(win32::get_error_msg(GetLastError()));
    }
}

void disable_raw_mouse_input()
{
    // TODO: unregister device?
    ctx.is_raw_mouse_input_enabled = false;
}

void query_monitor_info()
{
    sys_specs.monitors.clear();

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    // FIX: main monitor should always be available at the index 0

    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    u32 device_idx = 0;
    while (EnumDisplayDevicesA(0, device_idx, &dd, 0)) {
        hk::string name = dd.DeviceName;
        u32 monitor_idx = 0;
        while (EnumDisplayDevicesA(name.c_str(), monitor_idx, &dd, 0)) {
            // sys_specs.monitors.at(monitor_idx).name = dd.DeviceName;
            // sys_specs.monitors.at(monitor_idx).name += ", ";
            sys_specs.monitors.at(monitor_idx).name = dd.DeviceString;
            ++monitor_idx;
        }
        ++device_idx;
    }
}

b8 copy_to_clipboard(const std::string &target)
{
    if (!OpenClipboard(NULL)) { return false; }

    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, target.size() + 1);
    if (!hg) {
        CloseClipboard();
        return false;
    }

    LPVOID lock = GlobalLock(hg);
    memcpy(lock, target.c_str(), target.size() + 1);
    GlobalUnlock(hg);

    if (!SetClipboardData(CF_TEXT, hg)) {
        GlobalFree(hg);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return false;
}

void create_message_box(const std::string &name, const std::string &message)
{
    MessageBox(0, message.c_str(), name.c_str(), MB_ICONERROR);
}

void create_task_dialog(const std::string &title,
                        const std::string &header,
                        const std::string &message)
{
    std::wstring wtitle = hk::string_convert(title);
    std::wstring wheader = hk::string_convert(header);
    std::wstring wmessage = hk::string_convert(message);

    // TASKDIALOG_BUTTON buttons[] = {
    //     { IDOK, L"Continue" },
    //     { IDCANCEL, L"Abort" }
    // };

    /* ISSUE: Enabling hyperlinks when using content from an unsafe source
     * may cause security vulnerabilities */

    TASKDIALOGCONFIG config = {};
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_SIZE_TO_CONTENT;
    config.pszWindowTitle = wtitle.data();
    config.pszMainIcon = TD_ERROR_ICON;
    config.pszMainInstruction = wheader.data();
    config.pszContent = wmessage.data();
    // config.cButtons = ARRAYSIZE(buttons);
    // config.pButtons = buttons;

    config.pfCallback = TaskDialogCallback;

    TaskDialogIndirect(&config, NULL, NULL, NULL);
}

// void create_surface(void *instance, void *surface)
// {
//     const Window *win = static_cast<const Window *>(window);
//     VkWin32SurfaceCreateInfoKHR info = {};
//     info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
//     info.hwnd = win->hwnd();
//     info.hinstance = win->instance();
//     err = vkCreateWin32SurfaceKHR(instance, &info, 0, &surface);
// }

namespace win32 {

b8 prehook(Window *window, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        /* https://learn.microsoft.com/en-us/windows/win32/
         * inputdev/about-keyboard-input
         */
        b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

        u16 vkCode = LOWORD(wp);
        u16 keyFlags = HIWORD(lp);

        u16 scanCode = LOBYTE(keyFlags);
        b8 isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED;

        if (isExtendedKey)
            scanCode = MAKEWORD(scanCode, 0xE0);

        // if we want to distinguish these keys:
        switch (vkCode) {
        case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
        case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
        case VK_MENU:    // converts to VK_LMENU or VK_RMENU
            vkCode = LOWORD(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
            break;
        }

        hk::event::EventContext context;
        context.u16[0] = vkCode;
        context.u16[1] = static_cast<u16>(pressed);
        hk::event::fire(pressed ?
                        hk::event::EVENT_KEY_PRESSED :
                        hk::event::EVENT_KEY_RELEASED,
                        context);

        // Prevent window processing some keys
        return 0;
    } break;

    case WM_INPUT:
    {
        if(!ctx.is_raw_mouse_input_enabled) break;

        UINT size;
        GetRawInputData((HRAWINPUT)lp, RID_INPUT,
                        nullptr, &size, sizeof(RAWINPUTHEADER));

        hk::vector<RAWINPUT> raw_input(size);
        GetRawInputData((HRAWINPUT)lp, RID_INPUT,
                        raw_input.data(), &size, sizeof(RAWINPUTHEADER));

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(raw_input.data());

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            hk::event::EventContext context;
            context.i32[0] = raw->data.mouse.lLastX;
            context.i32[1] = raw->data.mouse.lLastY;
            hk::event::fire(hk::event::EVENT_RAW_MOUSE_MOVED, context);
        }
    } break;

    case WM_MOUSEMOVE:
    {
        if (ctx.is_raw_mouse_input_enabled) break;

        hk::event::EventContext context;
        context.i32[0] = GET_X_LPARAM(lp);
        context.i32[1] = GET_Y_LPARAM(lp);
        hk::event::fire(hk::event::EVENT_MOUSE_MOVED, context);
    } break;

    case WM_MOUSEWHEEL:
    {
        i16 zDelta = GET_WHEEL_DELTA_WPARAM(wp);
        if (zDelta != 0) {
            hk::event::EventContext context;
            context.i16[0] = (zDelta < 0) ? -1 : 1;
            hk::event::fire(hk::event::EVENT_MOUSE_WHEEL, context);
        }
    } break;

    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    {
        b8 pressed = (msg == WM_RBUTTONDOWN);

        // pressed ? SetCapture(hwnd) : ReleaseCapture();

        hk::event::EventContext context;
        context.u16[0] = VK_RBUTTON;
        context.u16[1] = static_cast<u16>(pressed);
        hk::event::fire(pressed ?
                        hk::event::EVENT_MOUSE_PRESSED :
                        hk::event::EVENT_MOUSE_RELEASED,
                        context);
    } break;

    case WM_MBUTTONUP:
    case WM_MBUTTONDOWN:
    {
        b8 pressed = (msg == WM_MBUTTONDOWN);

        hk::event::EventContext context;
        context.u16[0] = VK_MBUTTON;
        context.u16[1] = static_cast<u16>(pressed);
        hk::event::fire(pressed ?
                        hk::event::EVENT_MOUSE_PRESSED :
                        hk::event::EVENT_MOUSE_RELEASED,
                        context);
    } break;

    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    {
        b8 pressed = (msg == WM_LBUTTONDOWN);

        // pressed ? SetCapture(hwnd) : ReleaseCapture();

        hk::event::EventContext context;
        context.u16[0] = VK_LBUTTON;
        context.u16[1] = static_cast<u16>(pressed);
        hk::event::fire(pressed ?
                        hk::event::EVENT_MOUSE_PRESSED :
                        hk::event::EVENT_MOUSE_RELEASED,
                        context);
    } break;

    // default:
    }

    return false;
}

} // win32

} // hk::pltf

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    hk::platform::win32::Window *self;

    if (msg == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        self = static_cast<hk::platform::win32::Window*>(lpcs->lpCreateParams);
        self->hwnd = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<hk::platform::win32::Window*>(
            GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    // Engine-wide hook
    if (hk::platform::win32::prehook(self, msg, wParam, lParam)) { return 0; }

    // Per-window hook
    if (self && self->proc != nullptr) {
        return self->proc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);

}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor, (void)lprcMonitor, (void)dwData;

    hk::platform::MonitorSpec out;

    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(hMonitor, &info);

    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);

    // It returns native resolution in any case,
    // even if the OS tries to lie due to the DPI awareness of the process
    out.width = devmode.dmPelsWidth;
    out.height = devmode.dmPelsHeight;

    // Always >= 96
    u32 win32dpi = GetDpiForSystem();

    // Values can be negative if not the primary monitor
    i32 virtual_width = info.rcMonitor.right - info.rcMonitor.left;

    f32 virtual_to_real_ratio = virtual_width / static_cast<f32>(out.width);

    out.scale = win32dpi / 96.f / virtual_to_real_ratio;

    out.hz = devmode.dmDisplayFrequency;
    out.depth = devmode.dmBitsPerPel;

    hk::platform::sys_specs.monitors.push_back(out);

    return TRUE;
}

HRESULT CALLBACK TaskDialogCallback(HWND hwndFocus, UINT uNotification,
                                    WPARAM wParam, LPARAM lParam,
                                    LONG_PTR dwRefData)
{
    (void) hwndFocus; (void) wParam; (void) dwRefData;

    switch (uNotification) {
    case TDN_HYPERLINK_CLICKED: {
        PCWSTR pszHREF = (PCWSTR)lParam;
        ShellExecuteW(NULL, L"open", pszHREF, NULL, NULL, SW_SHOWNORMAL);
    } break;
    }

    return S_OK;
}

#endif // HKWIN32
