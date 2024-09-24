#ifndef HK_WIN_FILE_H
#define HK_WIN_FILE_H

#include "defines.h"
#include "utils/containers/hkvector.h"

namespace hk::platform {
    HKAPI b8 readFile(const std::string &path, hk::vector<u8>& out);

    HKAPI b8 findFile(const std::string &root, const std::string &target,
                      std::string *out = nullptr);
}

#endif // HK_WIN_FILE_H
