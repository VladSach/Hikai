#ifndef HK_WINLOG_H
#define HK_WINLOG_H

#include "win.h"
#include "hkcommon.h"
#include "hkstl/Logger.h"

#define HR_ERR_MSG(hr) getErrorMessage(hr)

inline std::string getErrorMessage(HRESULT hr)
{
    char buffer[4096] = {};
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer, _countof(buffer),
        nullptr);

    return buffer;
}

HKAPI void allocWinConsole();
HKAPI void deallocWinConsole();
HKAPI void setLogFile(const std::string &file);
HKAPI void removeLogFile();

#endif // HK_WINLOG_H
