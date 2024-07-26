#ifndef HK_WINDOW_H
#define HK_WINDOW_H

#include "defines.h"

#include <functional>
#include <string>

class Window {
public:
    virtual ~Window() {};

    virtual void init(std::wstring title, u32 width, u32 height) = 0;
    virtual void deinit() = 0;

    virtual b8 ProcessMessages() = 0;

    virtual u32 getWidth() const = 0;
    virtual u32 getHeight() const = 0;
    virtual b8  getIsVisible() const = 0;

    virtual void showCursor() = 0;
    virtual void hideCursor() = 0;
};

// Used for rendering without Window or
// if Window created after Application initialization
class EmptyWindow final : public Window {
public:
    ~EmptyWindow() { deinit(); };

    void init(std::wstring title, u32 width, u32 height)
    {
        winTitle = title;
        winWidth = width;
        winHeight = height;
        isVisible = true;
    }

    void deinit() {};

    b8 ProcessMessages() { return true; };

    u32 getWidth() const { return winWidth; };
    u32 getHeight() const { return winHeight; };
    b8  getIsVisible() const { return isVisible; };

    void showCursor() { return; };
    void hideCursor() { return; };

private:
    u32 winWidth = 0;
    u32 winHeight = 0;
    std::wstring winTitle = L"Mock Window";

    b8 isVisible = false;
};

#endif // HK_WINDOW_H
