#ifndef HK_LOCALE_H
#define HK_LOCALE_H

#include "platform/platform.h"

#include <string>

namespace hk {

inline std::string wstring_convert(const std::wstring &in)
{
    if (in.empty()) { return std::string(); }

    i32 sz = WideCharToMultiByte(
        CP_UTF8, 0,
        in.data(), -1,
        NULL, 0, NULL, NULL);

    std::string out(sz, 0);

    WideCharToMultiByte(
        CP_UTF8, 0,
        in.data(), -1,
        out.data(), sz,
        NULL, NULL);

    // FIX: stripping null terminator for convenience, shouldn't probably do this
    out.pop_back();

    return out;
}

}

#endif // HK_LOCALE_H
