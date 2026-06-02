#ifndef HK_WINDOW_H
#define HK_WINDOW_H

#include "platform/predef.h"

// Platform dependent includes
#if defined(HKWIN32)
#include "win.h"
#include <windowsx.h>
#elif defined(HKLINUX)
#endif

#include "hkcommon.h"
#include "utility/hktypes.h"
#include "strings/hkstring.h"

namespace hk {

class Window {
public:
    Window() = default;
    Window(const Window&) = delete;

    ~Window() { deinit(); }

    void init(hk::string title, u32 width, u32 height);
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

// Platform dependent public methods
#ifdef HKWIN32
    HWND hwnd() const { return hwnd_; }
    HINSTANCE instance() const { return hinstance_; }
#endif

private:
    u32 width_ = 400;
    u32 height_ = 400;
    hk::string title_ = "";

    b8 is_fullscreen_ = false;

    b8 is_cursor_enabled_ = true;
    b8 is_raw_mouse_input_enabled = false;

    enum class State {
        MAXIMIZED,
        MINIMIZED,
        RESTORED
    } state_ = State::RESTORED;

// Platform dependent members
#ifdef HKWIN32
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
#elif defined(HKLINUX)


#endif

// Platform dependent private methods
#ifdef HKWIN32
protected:
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
#endif
};

}

#endif // HK_WINDOW_H
