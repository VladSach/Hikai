#ifndef HK_WIN32_UTILS_H
#define HK_WIN32_UTILS_H

#include "win32.h"

namespace hk::platform::win32 {

inline hk::string get_error_msg(DWORD dw)
{
    char buffer[4096] = {};
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer, _countof(buffer),
        nullptr);

    return buffer;
}

}; // hk::platform::win32

#endif // HK_WIN32_UTILS_H
