#ifndef HK_WINWINDOW_H
#define HK_WINWINDOW_H

#include "win.h"
#include <windowsx.h>

#include "defines.h"

#include <functional>
#include <string>

class Window {
private:
    u32 winWidth = 400;
    u32 winHeight = 400;
    std::wstring winTitle = L"";

	b8 isFullscreen = false;
	b8 isVisible = false;
	u32 backgroundColor = 0xFFFFFFFF;

	HWND hWnd = nullptr;
	HINSTANCE hInstance = nullptr;
	
	// using OnResizeCallback = void(*)(int, int);
	// TODO: change this to "executeCallback"
	using OnResizeCallback = std::function<void(int, int)>;
	OnResizeCallback onResizeCallback;

protected:
	static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message,
											 WPARAM wParam, LPARAM lParam);
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

public:
	Window() = default;
	Window(const Window&) = delete;

	void init(std::wstring title, u32 width, u32 height);
	void deinit();

	b8 ProcessMessages();

	void flush();

	void setOnResizeCallback(const OnResizeCallback& callback)
	{
		onResizeCallback = callback;
	}

	u32 getWidth() const { return winWidth; }
	u32 getHeight() const { return winHeight; }
	b8  getIsVisible() const { return isVisible; }

	HWND getHWnd() { return hWnd; }
};

#endif // HK_WINWINDOW_H
