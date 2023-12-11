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
private:
    u32 winWidth = 400;
    u32 winHeight = 400;
    std::wstring winTitle = L"";

    b8 isFullscreen = false;
    b8 isVisible = false;

    HWND hWnd = nullptr;
    HINSTANCE hInstance = nullptr;

    EventSystem *evs = nullptr;

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

    u32 getWidth() const { return winWidth; }
    u32 getHeight() const { return winHeight; }
    b8  getIsVisible() const { return isVisible; }

    HWND getHWnd() const { return hWnd; }
    HINSTANCE getHInstance() const { return hInstance; }
};

#endif // HK_WINWINDOW_H
