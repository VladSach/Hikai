#ifndef HK_LOCALE_H
#define HK_LOCALE_H

#include "platform/platform.h"

#include <string>

namespace hk {

inline std::string wstring_convert(const std::wstring &in)
{
    if (in.empty()) { return std::string(); }

    u32 sz = WideCharToMultiByte(
        CP_UTF8, 0,
        in.data(), static_cast<int>(in.size()),
        NULL, 0, NULL, NULL);

    std::string out(sz, 0);

    WideCharToMultiByte(
        CP_UTF8, 0,
        in.data(), static_cast<int>(in.size()),
        out.data(), sz,
        NULL, NULL);

    return out;
}

}

#endif // HK_LOCALE_H
