#include "WinLog.h"

#include <fstream>
#include <iomanip>

void WinLog::allocWinConsole()
{
    deallocWinConsole();

	AllocConsole();

	hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
									     0, NULL,
										 CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);

	SetConsoleTitle("Hikai DevConsole");

	// Enable Virtual Terminal Sequences(colors, etc)
	// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);

	// TODO: change all console settings related stuff to virtual terminal

    constexpr u16 maxBufferSize = 100;
	// 95 char for info + 65 for message + 10 just in case
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

    Logger::getInstance()->addMessageHandler(this, logWinConsole);
}

void WinLog::deallocWinConsole()
{
	if (!hConsole) { return; }

	CloseHandle(hConsole);
	hConsole = 0;

	FreeConsole();

    Logger::getInstance()->removeMessageHandler(this, logWinConsole);
}

void WinLog::logWinConsole(void *self,
						   const Logger::MsgInfo& info,
                           const Logger::MsgAddInfo &misc)
{
	/* Colored output
	 * time [log_lvl]: caller message [opt]: args file line
	 * gray diff cyan white white red red
	 * More info about colors: https://ss64.com/nt/syntax-ansi.html
	 * */

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

	// New string with 95 empty spaces + delimiter
	constexpr char const *filler = "\n"
								   "                                          "
								   "                                          "
								   "           + ";
    wss << "\033[1;97m";
    	// << std::setw(65) << (info.message + " " + info.args).c_str()
		std::string message = info.message + " " + info.args;
		if (message.length() > 65) {
			u32 rows = static_cast<u32>(message.length()) / 65;
			for (u32 i = 0; i < rows; i++) {
				message.insert((95 * i) + 65 * (i + 1), filler);
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
	WriteConsoleW(static_cast<WinLog*>(self)->hConsole, wss.str().c_str(), length,
				  &dwBytesWritten, NULL);
}

void WinLog::logWinFile(void *self,
						const Logger::MsgInfo& info,
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
    wss << std::setw(30) << (info.message + " " + info.args).c_str() << ' ';
	wss << std::setw(12) << (misc.is_error ? misc.file.c_str() : "");
	wss << std::setw(3)  << (misc.is_error ? info.lineNumber.c_str() : "") << ' ';
	wss << '\n';

	std::wofstream file(static_cast<WinLog*>(self)->logFile, std::ios::out | std::ios::app);
    if (file.is_open()) {
        file << wss.rdbuf();
        file.close();
    }
}

bool WinLog::setConsoleSize(i16 cols, i16 rows)
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
