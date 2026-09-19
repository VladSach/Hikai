#include "platform/platform.h"
#ifdef TMPTURNOFF_HKWIN32

#include "Window.h"

#include "vendor/imgui/imgui_impl_win32.h"

#include <winuser.h>

namespace hk {

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT CALLBACK Window::StaticWindowProc(HWND hWnd, UINT message,
                                          WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
        return true;
    }

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
 

    default:
        result = DefWindowProc(hwnd_, message, wParam, lParam);
    }

    return result;
}

}

#endif // HKWIN32
