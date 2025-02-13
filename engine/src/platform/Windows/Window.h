#ifndef HK_WINWINDOW_H
#define HK_WINWINDOW_H

#include "win.h"
#include <windowsx.h>

#include "defines.h"

#include <string>

class Window {
public:
    Window() = default;
    Window(const Window&) = delete;

    ~Window() { deinit(); }

    void init(std::string title, u32 width, u32 height);
    void deinit();

    b8 ProcessMessages();

    /* Mouse settings */
    HKAPI void hideCursor();
    HKAPI void showCursor();

    HKAPI void lockCursor();
    HKAPI void unlockCursor();

    HKAPI void enableRawMouseInput();
    HKAPI void disableRawMouseInput();

public:
    u32 width() const { return width_; }
    u32 height() const { return height_; }
    b8  isVisible() const { return state_ != State::MINIMIZED; }

    HWND hwnd() const { return hwnd_; }
    HINSTANCE instance() const { return hinstance_; }

private:
    u32 width_ = 400;
    u32 height_ = 400;
    std::string title_ = "";

    b8 is_fullscreen_ = false;

    b8 is_cursor_enabled_ = true;
    b8 is_raw_mouse_input_enabled = false;

    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;

    enum class State {
        MAXIMIZED,
        MINIMIZED,
        RESTORED
    } state_ = State::RESTORED;

protected:
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif // HK_WINWINDOW_H
