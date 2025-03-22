#include "WinLog.h"

#include <fstream>
#include <iomanip>

// console log
static HANDLE hConsole = nullptr;
static u32 hndl_console = 0;

static constexpr u32 MaxFuncNameLength = 45;

// file log
static std::string log_file = "";
static u32 hndl_file = 0;

b8 setConsoleSize(i16 cols, i16 rows);
void logWinConsole(const hk::log::Log &log);
void logWinFile(const hk::log::Log &log);

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

void allocWinConsole()
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

void deallocWinConsole()
{
    if (!hConsole) { return; }

    CloseHandle(hConsole);
    hConsole = 0;

    FreeConsole();

    hk::log::removeMessageHandler(hndl_console);
}

void setLogFile(const std::string &file)
{
    log_file = file;
    if (log_file.empty()) { return; }

    hndl_file = hk::log::addMessageHandler(logWinFile);
}

void removeLogFile()
{
    hk::log::removeMessageHandler(hndl_file);
}

void logWinConsole(const hk::log::Log &log)
{
    /* Colored output
     * time [log_lvl]: caller message [opt]: args file line
     * gray diff cyan white white red red
     * More info about colors: https://ss64.com/nt/syntax-ansi.html
     * */

    if (!hConsole) { return; }

    constexpr char const *lookup_color[] =
    {
        "0;41m",
        "1;31m",
        "1;33m",
        "1;32m",
        "1;34m",
        "1;30m"
    };

    b8 is_error = log.level  < hk::log::Level::LVL_WARN;
    b8 is_trace = log.level == hk::log::Level::LVL_TRACE;

    std::wstringstream wss;
    wss << std::left;
    wss << "\033[1;90m" << log.time.c_str() << "\033[0;10m" << ' ';

    wss << "\033[" << lookup_color[static_cast<u32>(log.level)]
        << std::setw(8) << hk::log::levelToString(log.level).c_str()
        << "\033[0;10m" << ' ';

    std::string caller = log.caller;
    if (caller.size() > MaxFuncNameLength) {
        caller.erase(MaxFuncNameLength - 3, std::string::npos);
        caller.append("...");
    }

    wss << "\033[1;36m"
        << std::setw(MaxFuncNameLength) << caller.c_str()
        << "\033[0;10m" << ' ';

    if (is_trace) {
        wss << "\033[1;97m"
            <<  "---" << ' '
            << "\033[0;10m";
    }

    // Max message length is 160 chars
    wss << "\033[1;97m";
        std::string message = log.args;
        u64 msg_length = message.length();
        if (msg_length > 85) {
            u64 pos = 0;
            std::string row, token;
            std::string delimiter = " ";
            while ((pos = message.find(delimiter)) != std::string::npos) {
                token = message.substr(0, pos);
                if (row.length() + token.length() >= 150) {
                    wss << "\n   + " << row.c_str();
                    row.clear();
                }
                row.append(token);
                row.push_back(' ');
                message.erase(0, pos + delimiter.length());
            }
            wss << "\n   + " << row.c_str() << message.c_str();
        } else {
            wss << message.c_str();
        }
    wss << "\033[0;10m";

    wss << "\033[1;31m"
        << (msg_length > 85 ? "\n   -> " : "")
        << (is_error ? log.file.c_str() : "")
        << (is_error ? log.line.c_str() : "")
    << "\033[0;10m" << ' ';

    wss << '\n';

    DWORD dwBytesWritten = 0;
    u32 length = static_cast<unsigned>(wss.str().size());
    WriteConsoleW(hConsole, wss.str().c_str(), length, &dwBytesWritten, NULL);
}

void logWinFile(const hk::log::Log &log)
{
    // TODO: test and fix log to file
    // No coloring
    std::wstringstream wss;
    wss << std::left;
    wss << log.time.c_str() << ' ';
    wss << std::setw(8) << hk::log::levelToString(log.level).c_str() << ' ';
    std::string caller = log.caller;
    if (caller.size() > MaxFuncNameLength) {
        caller.erase(MaxFuncNameLength - 3, std::string::npos);
        caller.append("...");
    }
    wss << std::setw(MaxFuncNameLength) << caller.c_str() << ' ';
    // if (is_trace) { wss << "---" << ' '; }
    // wss << std::setw(40) << (info.message + " " + info.args).c_str() << ' ';
    wss << std::setw(30) << (log.args).c_str() << ' ';

    b8 is_error = log.level  < hk::log::Level::LVL_WARN;
    b8 is_trace = log.level == hk::log::Level::LVL_TRACE;
    wss << std::setw(12) << (is_error ? log.file.c_str() : "");
    wss << std::setw(3)  << (is_trace ? log.line.c_str() : "") << ' ';
    wss << '\n';

    std::wofstream file(log_file, std::ios::app);
    if (file.is_open()) {
        file << wss.rdbuf();
        file.close();
    }
}

b8 setConsoleSize(i16 cols, i16 rows)
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
