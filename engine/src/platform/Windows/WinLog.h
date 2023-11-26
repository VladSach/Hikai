#ifndef HK_WINLOG_H
#define HK_WINLOG_H

#include "win.h"
#include "defines.h"
#include "utils/Logger.h"

#define HR_ERR_MSG(hr) getErrorMessage(hr)

inline std::string getErrorMessage(HRESULT hr)
{
    char buffer[4096] = {};
    if(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                      nullptr,
                      hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      buffer,
                      _countof(buffer),
                      nullptr))
    {
        return buffer;
    }

    return buffer;
}

class WinLog {
private:
    // console log
    HANDLE hConsole = nullptr;

    // file log
    std::string logFile = "log.txt";

public:
    HKAPI void allocWinConsole();
    HKAPI void deallocWinConsole();

    HKAPI void setLogFile(std::string file) { logFile = file; }

    static void logWinConsole(void *self,
                              const Logger::MsgInfo& info,
                              const Logger::MsgAddInfo &misc);

    static void logWinFile(void *self,
                           const Logger::MsgInfo& info,
                           const Logger::MsgAddInfo &misc);

private:
    bool setConsoleSize(i16 cols, i16 rows);
};

#endif // HK_WINLOG_H
