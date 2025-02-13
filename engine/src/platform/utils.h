#ifndef HK_PLATFORM_UTILS_H
#define HK_PLATFORM_UTILS_H

#include "defines.h"

#include <string>

namespace hk::platform {

HKAPI b8 copyToClipboard(const std::string &target);
HKAPI void addMessageBox(const std::string &name,
                         const std::string &message);

}

#endif // HK_PLATFORM_UTILS_H
