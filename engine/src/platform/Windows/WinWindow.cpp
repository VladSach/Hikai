#include "WinWindow.h"

#include "defines.h"
#include "utils/Logger.h"
#include "platform/PlatformArgs.h"

#include <winuser.h>
#include <string>

void Window::init(std::wstring title, u32 width, u32 height)
{
	winTitle = title;
	winWidth = width;
	winHeight = height;

	hInstance = PlatformArgs::instance()->hInstance;

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = Window::StaticWindowProc;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "MainWinClass";

	// register the window class
	ALWAYS_ASSERT(RegisterClassEx(&wc), "Failed to register the window");

    u32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    u32 screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (isFullscreen) {

        // TODO: add Fullscreen
    }

    // set the size, but not the position
	RECT wr = { 0, 0,
	            static_cast<LONG>(winWidth),
	            static_cast<LONG>(winHeight) };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	hWnd = CreateWindowExW(
		NULL,
		L"MainWinClass", // name of the window class
		std::wstring(L"Hikai (" + winTitle + L")").c_str(), // title
		WS_OVERLAPPEDWINDOW,
		(screenWidth  - wr.right)  / 2, // x-position of the window
		(screenHeight - wr.bottom) / 2, // y-position of the window
		wr.right - wr.left,    // width of the window
		wr.bottom - wr.top,    // height of the window
		NULL, NULL, hInstance, (LPVOID)this
	);

	ShowWindow(hWnd, SW_SHOWNORMAL);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

	LOG_INFO("Window is successfully created");
}

void Window::deinit()
{
    UnregisterClassW(L"MainWinClass", hInstance);
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

void Window::flush()
{
	// swapchain->Present(0, 0);
}

// Forward declare message handler from imgui_impl_win32.cpp
// extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
// 															 UINT msg,
// 															 WPARAM wParam,
// 															 LPARAM lParam);

LRESULT CALLBACK Window::StaticWindowProc(HWND hWnd, UINT message,
										  WPARAM wParam, LPARAM lParam)
{
	Window *self;
	if (message == WM_NCCREATE) {
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		self = static_cast<Window*>(lpcs->lpCreateParams);
		self->hWnd = hWnd;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	} else {
		self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (self) {
		LRESULT result = self->WindowProc(message, wParam, lParam);
		return result;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Window::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    //     return true;

    LRESULT result = 0;
    // sort through and find what code to run for the message given
    switch (message) {
    // this message is read when the window is closed
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // close the application entirely
        PostQuitMessage(0);
        break;

    case WM_SIZE:
	{
        if (wParam == SIZE_MINIMIZED) {
            // Window is being minimized
            isVisible = true;
            LOG_INFO("Window minimized");
            break;
        }

		RECT cr;
		GetClientRect(hWnd, &cr);
		int width = cr.right - cr.left;
		int height = cr.bottom - cr.top;

		winWidth = width;
		winHeight = height;
		isVisible = false;

		if (onResizeCallback) { onResizeCallback(width, height); }

		LOG_INFO("Window's size set to: ", width, "x", height);
	} break;

    default:
        result = DefWindowProc(hWnd, message, wParam, lParam);
    }

    return result;
}
