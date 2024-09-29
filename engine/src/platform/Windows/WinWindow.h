#ifndef HK_WINWINDOW_H
#define HK_WINWINDOW_H

#include "win.h"
#include <windowsx.h>

#include "defines.h"
#include "platform/Window.h"
#include "core/EventSystem.h"

#include <functional>
#include <string>

class WinWindow final : public Window {
protected:
    static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message,
                                             WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
    HKAPI WinWindow() = default;
    WinWindow(const WinWindow&) = delete;

    ~WinWindow() { deinit(); }

    void init(std::wstring title, u32 width, u32 height);
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
    u32 getWidth() const { return winWidth; }
    u32 getHeight() const { return winHeight; }
    b8  getIsVisible() const { return isVisible; }

    HWND getHWnd() const { return hWnd; }
    HINSTANCE getHInstance() const { return hInstance; }

private:
    u32 winWidth = 400;
    u32 winHeight = 400;
    std::wstring winTitle = L"";

    b8 isFullscreen = false;
    b8 isVisible = false;

    b8 cursorEnabled = true;
    b8 rawMouseInputEnabled = false;

    HWND hWnd = nullptr;
    HINSTANCE hInstance = nullptr;

    hk::EventSystem *evs = nullptr;
};

#endif // HK_WINWINDOW_H
