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
	evs = EventSystem::instance();

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
    LRESULT result = 0;

    switch (message) {
    case WM_CLOSE:
    	// this message is read when the window is closed
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
            isVisible = false;
            LOG_INFO("Window minimized");
            break;
        }

		RECT cr;
		GetClientRect(hWnd, &cr);
		u32 width = cr.right - cr.left;
		u32 height = cr.bottom - cr.top;

		winWidth = width;
		winHeight = height;

		hk::EventContext context;
		context.u32[0] = width;
		context.u32[1] = height;
		evs->fireEvent(hk::EVENT_WINDOW_RESIZE, context);

		LOG_INFO("Window's size set to: ", width, "x", height);
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

		hk::EventContext context;
		context.u16[0] = vkCode;
		context.u16[1] = static_cast<u16>(pressed);
        evs->fireEvent(pressed ? hk::EVENT_KEY_PRESSED : hk::EVENT_KEY_RELEASED,
        			   context);

        // Prevent window processing some keys
        return 0;
	} break;

	case WM_MOUSEMOVE:
	{
		hk::EventContext context;
		context.i32[0] = GET_X_LPARAM(lParam);
		context.i32[1] = GET_Y_LPARAM(lParam);
        evs->fireEvent(hk::EVENT_MOUSE_MOVED, context);
	} break;

	case WM_MOUSEWHEEL:
	{
		i16 zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (zDelta != 0) {
			hk::EventContext context;
			context.i16[0] = (zDelta < 0) ? -1 : 1;
        	evs->fireEvent(hk::EVENT_MOUSE_WHEEL, context);
        }
	} break;

    case WM_RBUTTONUP:
	case WM_RBUTTONDOWN:
    {
        b8 pressed = (message == WM_RBUTTONDOWN);

		hk::EventContext context;
		context.u16[0] = VK_RBUTTON;
		context.u16[1] = static_cast<u16>(pressed);
        evs->fireEvent(pressed ? hk::EVENT_MOUSE_PRESSED :
                                 hk::EVENT_MOUSE_RELEASED,
        			   context);
    } break;

    case WM_MBUTTONUP:
	case WM_MBUTTONDOWN:
    {
        b8 pressed = (message == WM_MBUTTONDOWN);

		hk::EventContext context;
		context.u16[0] = VK_MBUTTON;
		context.u16[1] = static_cast<u16>(pressed);
        evs->fireEvent(pressed ? hk::EVENT_MOUSE_PRESSED :
                                 hk::EVENT_MOUSE_RELEASED,
        			   context);
    } break;

    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    {
        b8 pressed = (message == WM_LBUTTONDOWN);

		hk::EventContext context;
		context.u16[0] = VK_LBUTTON;
		context.u16[1] = static_cast<u16>(pressed);
        evs->fireEvent(pressed ? hk::EVENT_MOUSE_PRESSED :
                                 hk::EVENT_MOUSE_RELEASED,
        			   context);
    } break;

    default:
        result = DefWindowProc(hWnd, message, wParam, lParam);
    }

    return result;
}
