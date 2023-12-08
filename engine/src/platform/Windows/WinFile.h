#ifndef HK_WIN_FILE_H
#define HK_WIN_FILE_H

#include "defines.h"
#include "utils/containers/hkvector.h"

namespace hk::platform {
    b8 readFile(const std::string &path, hk::vector<u8>& out);
}

#endif // HK_WIN_FILE_H
