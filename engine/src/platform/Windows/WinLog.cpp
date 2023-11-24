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

	// Enable colors
    DWORD dwMode = 0;
    GetConsoleMode(hConsole, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);

	// Enable scrolling
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	COORD bufferSize = csbi.dwSize;
	bufferSize.Y = maxBufferSize;
	SetConsoleScreenBufferSize(hConsole, bufferSize);

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
		<< std::setw(12) << misc.caller.c_str()
		<< "\033[0;10m" << ' ';

	if (misc.is_trace) {
		wss << "\033[1;97m"
			<<  "---" << ' '
			<< "\033[0;10m";
	}

    wss << "\033[1;97m"
    	<< std::setw(40) << (info.message + " " + info.args).c_str()
    	<< "\033[0;10m" << ' ';

	wss << "\033[1;31m"
		<< std::setw(12) << (misc.is_error ? misc.file.c_str() : "")
		<< "\033[0;10m";

	wss << "\033[1;31m"
		<< std::setw(3) << (misc.is_error ? info.lineNumber.c_str() : "")
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
	// No coloring
	std::wstringstream wss;
	wss << std::left;
    wss << misc.time.c_str() << ' ';
    wss << std::setw(8)  << Logger::lookup_level[misc.log_lvl] << ' ';
	wss << std::setw(12) << misc.caller.c_str() << ' ';
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
