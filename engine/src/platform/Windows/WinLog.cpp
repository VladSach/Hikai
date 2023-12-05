#include "WinLog.h"

#include <fstream>
#include <iomanip>

// console log
static HANDLE hConsole = nullptr;
// file log
static std::string logFile = "";

bool setConsoleSize(i16 cols, i16 rows);
void logWinConsole(void *self,
                   const Logger::MsgInfo& info,
                   const Logger::MsgAddInfo &misc);
void logWinFile(void *self,
                const Logger::MsgInfo& info,
                const Logger::MsgAddInfo &misc);

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    {
        /* Turns out it's too late to call FreeConsole in the handler
         * so this won't work for CTRL_CLOSE_EVENT
         * if console should be closed w/o closing the app user can press Ctrl+C
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

void allocWinConsole()
{
    if (hConsole) { return; }

    AllocConsole();

    hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                         0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);

    SetConsoleTitle("Hikai DevConsole");

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // Enable Virtual Terminal Sequences(colors, etc)
    // https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
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

    Logger::getInstance()->addMessageHandler(nullptr, logWinConsole);
}

void deallocWinConsole()
{
    if (!hConsole) { return; }

    CloseHandle(hConsole);
    hConsole = 0;

    FreeConsole();

    Logger::getInstance()->removeMessageHandler(nullptr, logWinConsole);
}


void setLogFile(const std::string &file)
{
    logFile = file;
    if (logFile.empty()) { return; }

    Logger::getInstance()->addMessageHandler(nullptr, logWinFile);
}

void removeLogFile()
{
    Logger::getInstance()->removeMessageHandler(nullptr, logWinFile);
}

void logWinConsole(void *self,
                   const Logger::MsgInfo& info,
                   const Logger::MsgAddInfo &misc)
{
    /* Colored output
     * time [log_lvl]: caller message [opt]: args file line
     * gray diff cyan white white red red
     * More info about colors: https://ss64.com/nt/syntax-ansi.html
     * */

    // Unused
    (void)self;

    if (!hConsole) { return; }

    constexpr char const *lookup_color[static_cast<int>(Logger::Level::max_levels)] = {
        "0;41m",
        "1;31m",
        "1;33m",
        "1;32m",
        "1;34m",
        "1;30m"
    };

    std::wstringstream wss;
    wss << std::left;
    wss << "\033[1;90m" << misc.time.c_str() << "\033[0;10m" << ' ';

    wss << "\033[" << lookup_color[misc.log_lvl]
        << std::setw(8) << Logger::lookup_level[misc.log_lvl]
        << "\033[0;10m" << ' ';

    wss << "\033[1;36m"
        << std::setw(Logger::MaxFuncNameLength) << misc.caller.c_str()
        << "\033[0;10m" << ' ';

    if (misc.is_trace) {
        wss << "\033[1;97m"
            <<  "---" << ' '
            << "\033[0;10m";
    }

    // New string with 75 empty spaces + delimiter
    constexpr char const *filler = "\n"
                                   "                                          "
                                   "                                 + ";
    wss << "\033[1;97m";
        // << std::setw(65) << (info.message + " " + info.args).c_str()
        std::string message = info.args;
        if (message.length() > 85) {
            u32 rows = static_cast<u32>(message.length()) / 75;
            for (u32 i = 0; i < rows; i++) {
                message.insert((75 * i) + 85 * (i + 1), filler);
            }
        }
        wss << message.c_str();
    wss << "\033[0;10m";

    wss << (misc.is_error ? filler : "");

    wss << "\033[1;31m"
        // << std::setw(12) << (misc.is_error ? misc.file.c_str() : "")
        // << std::setw(3) << (misc.is_error ? info.lineNumber.c_str() : "")
        << (misc.is_error ? misc.file.c_str() : "")
        << (misc.is_error ? info.lineNumber.c_str() : "")
        << "\033[0;10m" << ' ';

    wss << '\n';

    DWORD dwBytesWritten = 0;
    unsigned length = static_cast<unsigned>(wss.str().size());
    WriteConsoleW(hConsole, wss.str().c_str(), length, &dwBytesWritten, NULL);
}

void logWinFile(void *self,
                const Logger::MsgInfo& info,
                const Logger::MsgAddInfo &misc)
{
    // Unused
    (void)self;

    if (!hConsole) { return; }

    // TODO: test and fix log to file
    // No coloring
    std::wstringstream wss;
    wss << std::left;
    wss << misc.time.c_str() << ' ';
    wss << std::setw(8)  << Logger::lookup_level[misc.log_lvl] << ' ';
    wss << std::setw(Logger::MaxFuncNameLength) << misc.caller.c_str() << ' ';
    // if (is_trace) { wss << "---" << ' '; }
    // wss << std::setw(40) << (info.message + " " + info.args).c_str() << ' ';
    wss << std::setw(30) << (info.args).c_str() << ' ';
    wss << std::setw(12) << (misc.is_error ? misc.file.c_str() : "");
    wss << std::setw(3)  << (misc.is_error ? info.lineNumber.c_str() : "") << ' ';
    wss << '\n';

    std::wofstream file(logFile, std::ios::out | std::ios::app);
    if (file.is_open()) {
        file << wss.rdbuf();
        file.close();
    }
}

bool setConsoleSize(i16 cols, i16 rows)
{
    CONSOLE_FONT_INFO fi;
    CONSOLE_SCREEN_BUFFER_INFO bi;
    int w, h, bw, bh;
    RECT rect = {0, 0, 0, 0};
    COORD coord = {0, 0};

    HWND hWnd = GetConsoleWindow();
    if (hWnd) {
        if (GetCurrentConsoleFont(hConsole, FALSE, &fi)) {
            if (GetClientRect(hWnd, &rect)) {
                w = rect.right-rect.left;
                h = rect.bottom-rect.top;
                if (GetWindowRect(hWnd, &rect)) {
                    bw = rect.right-rect.left-w;
                    bh = rect.bottom-rect.top-h;
                    if (GetConsoleScreenBufferInfo(hConsole, &bi)) {
                        coord.X = bi.dwSize.X;
                        coord.Y = bi.dwSize.Y;
                        if (coord.X < cols || coord.Y < rows) {
                            if (coord.X < cols) {
                                coord.X = cols;
                            }
                            if (coord.Y < rows) {
                                coord.Y = rows;
                            }
                            if (!SetConsoleScreenBufferSize(hConsole, coord)) {
                                return false;
                            }
                        }
                        return SetWindowPos(hWnd, NULL, rect.left, rect.top,
                                            cols*fi.dwFontSize.X+bw,
                                            rows*fi.dwFontSize.Y+bh,
                                            SWP_NOACTIVATE |
                                            SWP_NOMOVE |
                                            SWP_NOOWNERZORDER |
                                            SWP_NOZORDER);
                    }
                }
            }
        }
    }
    return false;
}
