#include "WinLog.h"

#include <fstream>
#include <iomanip>

// console log
static HANDLE hConsole = nullptr;
// file log
static std::string logFile = "";

bool setConsoleSize(i16 cols, i16 rows);
void logWinConsole(const Logger::MsgInfo& info,
                   const Logger::MsgAddInfo &misc);
void logWinFile(const Logger::MsgInfo& info,
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

    Logger::getInstance()->addMessageHandler(logWinConsole);
}

void deallocWinConsole()
{
    if (!hConsole) { return; }

    CloseHandle(hConsole);
    hConsole = 0;

    FreeConsole();

    Logger::getInstance()->removeMessageHandler(logWinConsole);
}


void setLogFile(const std::string &file)
{
    logFile = file;
    if (logFile.empty()) { return; }

    Logger::getInstance()->addMessageHandler(logWinFile);
}

void removeLogFile()
{
    Logger::getInstance()->removeMessageHandler(logWinFile);
}

void logWinConsole(const Logger::MsgInfo& info,
                   const Logger::MsgAddInfo &misc)
{
    /* Colored output
     * time [log_lvl]: caller message [opt]: args file line
     * gray diff cyan white white red red
     * More info about colors: https://ss64.com/nt/syntax-ansi.html
     * */

    if (!hConsole) { return; }

    constexpr char const *
        lookup_color[static_cast<int>(Logger::Level::max_levels)] =
    {
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

    // Max message length is 160 chars
    wss << "\033[1;97m";
        std::string message = info.args;
        u64 msg_length = message.length();
        if (msg_length > 85) {
            // std::string row, word;
            // u32 row_length = 0;
            // for (u32 i = 0; i < message.length(); i++) {
            //     if (row_length <= 140 && message.at(i) != ' ') {
            //         word.push_back(message.at(i));
            //     } else if (row_length <= 140 && message.at(i) == ' ') {
            //         word.push_back(message.at(i));
            //         row.append(word);
            //         word.clear();
            //     } else if (row_length > 140) {
            //         wss << "\n   + " << row.c_str();
            //         row.clear();
            //         word.push_back(message.at(i));
            //         row_length = 0;
            //     }
            //
            //     ++row_length;
            // }

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
            wss << "\n   + " << row.c_str();
        } else {
            wss << message.c_str();
        }
    wss << "\033[0;10m";

    wss << "\033[1;31m"
        << (msg_length > 85 ? "\n   -> " : "")
        << (misc.is_error ? misc.file.c_str() : "")
        << (misc.is_error ? info.lineNumber.c_str() : "")
    << "\033[0;10m" << ' ';

    wss << '\n';

    DWORD dwBytesWritten = 0;
    unsigned length = static_cast<unsigned>(wss.str().size());
    WriteConsoleW(hConsole, wss.str().c_str(), length, &dwBytesWritten, NULL);
}

void logWinFile(const Logger::MsgInfo& info,
                const Logger::MsgAddInfo &misc)
{
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

    std::wofstream file(logFile, std::ios::app);
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
