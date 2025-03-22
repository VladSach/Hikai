#ifndef HK_PLATFORM_UTILS_H
#define HK_PLATFORM_UTILS_H

#include "hkcommon.h"

#include "hkstl/utility/hktypes.h"

#include <string>

namespace hk::platform {

HKAPI b8 copyToClipboard(const std::string &target);

HKAPI void addMessageBox(const std::string &title,
                         const std::string &message);
HKAPI void addTaskDialog(const std::string &title,
                         const std::string &header,
                         const std::string &message);

}

#endif // HK_PLATFORM_UTILS_H
