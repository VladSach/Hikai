#ifndef HK_LOCALE_H
#define HK_LOCALE_H

#include "platform/platform.h"

#include <string>
#include <stack>

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

inline std::string normalise(const std::string &path)
{
    // FIX: kinda works, but I don't like the way it's written
    std::stack<std::string> stack;

    std::string result;

    u32 start = 0;
    for (u32 i = 0; i < path.length(); ++i) {
        if (path[i] == '/' || path[i] == '\\') {
            std::string dir = path.substr(start, i - start);

            if (dir == ".." || dir == ".") {
                if (!stack.empty()) { stack.pop(); }
            } else {
                stack.push(dir);
            }

            start = i + 1;
        }
    }

    // Handle the last directory
    std::string dir = path.substr(start);
    if (!dir.empty() && dir != ".") {
        if (dir == "..") {
            if (!stack.empty()) {
                stack.pop();
            }
        } else {
            stack.push(dir);
        }
    }

    if (stack.size() == 1) {
        result = stack.top();
        stack.pop();
        return result;
    }

    while (!stack.empty()) {
        result = "/" + stack.top() + result;
        stack.pop();
    }

    return result;
}

}

#endif // HK_LOCALE_H
