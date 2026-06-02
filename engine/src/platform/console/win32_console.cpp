#include "platform/predef.h"
#ifdef HKWIN32

#include "console.h"

#include "console.h"

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    {
        /* INFO: Turns out it's too late to call FreeConsole in the handler
         * so this won't work for CTRL_CLOSE_EVENT
         * if console should be closed w/o closing the app user can Ctrl+C
         * or alternative implementations should be used.
         * Like creating child process and IO pipes
         * https://stackoverflow.com/questions/696117/
         * what-happens-when-you-close-a-c-console-application
         */
        deallocWinConsole();
        return TRUE;
    } break;

    default: { return FALSE; }
    }
}

namespace hk::platform {

static HANDLE hConsole = nullptr;
static u32 hndl_console = 0;

constexpr u32 MaxFuncNameLength = 45;

void alloc_console()
{
    if (hConsole) { return; }

    AllocConsole();

    hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                         0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);

    SetConsoleTitle("Hikai Log Console");

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // Enable Virtual Terminal Sequences(colors, etc)
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);

    // TODO: change all console settings related stuff to virtual terminal

    constexpr u16 maxBufferSize = 100;
    // 75 char for info + 85 for message + 10 just in case
    constexpr u16 maxBufferLineSize = 170;

    // Enable scrolling
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD bufferSize = csbi.dwSize;
    bufferSize.Y = maxBufferSize;
    bufferSize.X = maxBufferLineSize;
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    setConsoleSize(maxBufferLineSize, csbi.srWindow.Bottom);

    hndl_console = hk::log::addMessageHandler(logWinConsole);
}

void dealloc_console()
{
    if (!hConsole) { return; }

    CloseHandle(hConsole);
    hConsole = 0;

    FreeConsole();

    hk::log::removeMessageHandler(hndl_console);
}

void write_console(const char *text);
{
    DWORD dwBytesWritten = 0;
    WriteConsoleW(hConsole, text, strlen(text), &dwBytesWritten, NULL);
}

b8 set_console_size(i16 cols, i16 rows)
{
    CONSOLE_FONT_INFO fi;
    CONSOLE_SCREEN_BUFFER_INFO bi;
    i32 w, h, bw, bh;
    RECT rect = {0, 0, 0, 0};
    COORD coord = {0, 0};

    HWND hwnd = GetConsoleWindow();

    if (!hwnd || !GetCurrentConsoleFont(hConsole, FALSE, &fi)) {
        return false;
    }

    if (!GetClientRect(hwnd, &rect)) { return false; }

    w = rect.right-rect.left;
    h = rect.bottom-rect.top;

    if (!GetWindowRect(hwnd, &rect)) { return false; }

    bw = rect.right-rect.left-w;
    bh = rect.bottom-rect.top-h;

    if (!GetConsoleScreenBufferInfo(hConsole, &bi)) {
        return false;
    }

    coord.X = bi.dwSize.X;
    coord.Y = bi.dwSize.Y;

    if (coord.X < cols || coord.Y < rows) {

        if (coord.X < cols) { coord.X = cols; }
        if (coord.Y < rows) { coord.Y = rows; }

        if (!SetConsoleScreenBufferSize(hConsole, coord)) {
            return false;
        }
    }

    return SetWindowPos(hwnd, NULL, rect.left, rect.top,
                        cols * fi.dwFontSize.X + bw,
                        rows * fi.dwFontSize.Y + bh,
                        SWP_NOACTIVATE |
                        SWP_NOMOVE |
                        SWP_NOOWNERZORDER |
                        SWP_NOZORDER);
}

} // namespace hk::platform

#endif // HKWIN32
