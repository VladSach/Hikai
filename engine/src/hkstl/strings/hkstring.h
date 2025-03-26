#ifndef HK_STRING_H
#define HK_STRING_H

#include <string>

#include "containers/hkvector.h"

namespace hk {

// FIX: temp
using string = std::string;

/* ===== Utils ===== */

inline hk::vector<hk::string> strtok(const std::string &s, const std::string &delim)
{
    hk::vector<std::string> out;

    u64 start = 0;
    u64 end = s.find(delim);

    while (end != std::string::npos) {

        if (start != end) {
            out.push_back(s.substr(start, end - start));
        }

        start = end + delim.size();

        end = s.find(delim, start);
    }

    // Add last token if exists
    if (start != 0) {
        out.push_back(s.substr(start));
    }

    return out;
}

}

#endif // HK_STRING_H
