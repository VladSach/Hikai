#ifdef _WIN32
#include "Window.h"

#include "vendor/imgui/imgui_impl_win32.h"

#include "platform/platform.h"
#include "platform/args.h"

#include "core/events.h"

#include "hkstl/Logger.h"
#include "hkstl/containers/hkvector.h"

#include "utils/spec.h"

#include <winuser.h>
#include <hidusage.h>

#include <string>

void Window::init(std::string title, u32 width, u32 height)
{
    title_ = title;
    width_ = width;
    height_ = height;

    /* TODO: change based on win version + move to per monitor aware
     * SetProcessDPIAware() - Windows Vista and up, only for system aware (?)
     * SetProcessDpiAwareness() - Windows 8.1 and up
     * SetProcessDpiAwarenessContext() - Windows 10, version 1607 and up
     */

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

    hk::spec::MonitorSpec info = hk::spec::system().monitors.at(0);

    hinstance_ = hk::platform::args::hInstance;

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = Window::StaticWindowProc;
    wc.hInstance = hinstance_;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wc.lpszClassName = "Hikari Main Window";

    // register the window class
    ALWAYS_ASSERT(RegisterClassEx(&wc), "Failed to register the window");

    if (width_ > info.width) {
        width_ = info.width;
        LOG_WARN("Window width can not be bigger then screen width:",
                 width, "vs", info.width);
    }
    if (height_ > info.height) {
        height_ = info.height;
        LOG_WARN("Window height can not be bigger then screen height:",
                 height, "vs", info.height);
    }

    // Screen size minus taskbar
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    u32 workareaWidth = workArea.right - workArea.left;
    u32 workareaHeight = workArea.bottom - workArea.top;

    if (is_fullscreen_) {

        // TODO: add Fullscreen
    }

    // set the size, but not the position
    RECT wr = {
        0, 0,
        static_cast<LONG>(width_),
        static_cast<LONG>(height_)
    };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    CreateWindowEx(
        NULL,
        "Hikari Main Window",
        title_.c_str(),
        WS_OVERLAPPEDWINDOW,
        (workareaWidth  - wr.right)  / 2, // x-position of the window
        (workareaHeight - wr.bottom) / 2, // y-position of the window
        wr.right - wr.left,    // width of the window
        wr.bottom - wr.top,    // height of the window
        NULL, NULL, hinstance_, (LPVOID)this
    );

    ALWAYS_ASSERT(hwnd_, "Failed to create a Window");

    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);

    LOG_INFO("Window is successfully created");
}

void Window::deinit()
{
    UnregisterClassW(L"MainWinClass", hinstance_);
}

bool Window::ProcessMessages()
{
    MSG msg = { 0 };

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            return false;

        // translate keystroke messages into the right format
        TranslateMessage(&msg);

        // send the message to the WindowProc function
        DispatchMessage(&msg);
    }

    return true;
}

void Window::showCursor()
{
    is_cursor_enabled_ = true;
    while(ShowCursor(TRUE) < 0);
}

void Window::hideCursor()
{
    is_cursor_enabled_ = false;
    while(ShowCursor(FALSE) >= 0);
}

void Window::lockCursor()
{
    RECT r;
    GetWindowRect(hwnd_, &r);
    ClipCursor(&r);
}

void Window::unlockCursor()
{
    ClipCursor(nullptr);
}

void Window::enableRawMouseInput()
{
    if (is_raw_mouse_input_enabled) return;

    is_raw_mouse_input_enabled = true;

    BOOL result;

    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    // RIDEV_INPUTSINK - Enables the caller to receive the input
    // even when the caller is not in the foreground
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwnd_;
    result = RegisterRawInputDevices(rid, 1, sizeof(rid[0]));

    if (result == FALSE) {
        LOG_WARN("Failed to register raw mouse input");
    }
}

void Window::disableRawMouseInput()
{
    // TODO: unregister device?
    is_raw_mouse_input_enabled = false;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT CALLBACK Window::StaticWindowProc(HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    Window *self;
    if (message == WM_NCCREATE) {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        self = static_cast<Window*>(lpcs->lpCreateParams);
        // https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
        self->hwnd_ = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Window*>(
            GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (self) {
        LRESULT result = self->WindowProc(message, wParam, lParam);
        return result;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message) {
    case WM_CLOSE: {
        // this message is read when the window is closed
        DestroyWindow(hwnd_);
    } break;

    case WM_DESTROY: {
        // close the application entirely
        PostQuitMessage(0);
    } break;

    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED) {
            state_ = State::MINIMIZED;
            LOG_INFO("Window is minimized");
            break;
        }

        if (wParam == SIZE_MAXIMIZED) {
            state_ = State::MAXIMIZED;
            LOG_INFO("Window is maximized");
            PostMessage(hwnd_, WM_EXITSIZEMOVE, 0, 0);
            break;
        }

        if (state_ == State::MAXIMIZED) {
            if (wParam == SIZE_RESTORED) {
                LOG_INFO("Window is restored");
                PostMessage(hwnd_, WM_EXITSIZEMOVE, 0, 0);
            }
        }

        state_ = State::RESTORED;
    } break;

    case WM_EXITSIZEMOVE:
    {
        RECT cr;
        GetClientRect(hwnd_, &cr);
        u32 width = cr.right - cr.left;
        u32 height = cr.bottom - cr.top;

        if (width_ == width && height_ == height) {
            LOG_TRACE("Window moved");
            break;
        }

        width_ = width;
        height_ = height;

        hk::event::EventContext context;
        context.u32[0] = width;
        context.u32[1] = height;
        hk::event::fire(hk::event::EVENT_WINDOW_RESIZE, context);

        LOG_INFO("Window's size set to:", width_, "x", height_);
    } break;
 
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        /* https://learn.microsoft.com/en-us/windows/win32/
         * inputdev/about-keyboard-input
         */
        b8 pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

        u16 vkCode = LOWORD(wParam);
        u16 keyFlags = HIWORD(lParam);

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
        if(!is_raw_mouse_input_enabled) break;

        UINT size;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
                        nullptr, &size, sizeof(RAWINPUTHEADER));

        hk::vector<RAWINPUT> rawInputs(size);
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
                        rawInputs.data(), &size, sizeof(RAWINPUTHEADER));

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawInputs.data());

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            hk::event::EventContext context;
            context.i32[0] = raw->data.mouse.lLastX;
            context.i32[1] = raw->data.mouse.lLastY;
            hk::event::fire(hk::event::EVENT_RAW_MOUSE_MOVED, context);
        }
    } break;

    case WM_MOUSEMOVE:
    {
        if (is_raw_mouse_input_enabled) break;

        hk::event::EventContext context;
        context.i32[0] = GET_X_LPARAM(lParam);
        context.i32[1] = GET_Y_LPARAM(lParam);
        hk::event::fire(hk::event::EVENT_MOUSE_MOVED, context);
    } break;

    case WM_MOUSEWHEEL:
    {
        i16 zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (zDelta != 0) {
            hk::event::EventContext context;
            context.i16[0] = (zDelta < 0) ? -1 : 1;
            hk::event::fire(hk::event::EVENT_MOUSE_WHEEL, context);
        }
    } break;

    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    {
        b8 pressed = (message == WM_RBUTTONDOWN);

        if (pressed) {
            SetCapture(hwnd_);
        } else {
            ReleaseCapture();
        }

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
        b8 pressed = (message == WM_MBUTTONDOWN);

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
        b8 pressed = (message == WM_LBUTTONDOWN);

        if (pressed) {
            SetCapture(hwnd_);
        } else {
            ReleaseCapture();
        }

        hk::event::EventContext context;
        context.u16[0] = VK_LBUTTON;
        context.u16[1] = static_cast<u16>(pressed);
        hk::event::fire(pressed ?
                        hk::event::EVENT_MOUSE_PRESSED :
                        hk::event::EVENT_MOUSE_RELEASED,
                        context);
    } break;

    default:
        result = DefWindowProc(hwnd_, message, wParam, lParam);
    }

    return result;
}

#endif // _WIN32

