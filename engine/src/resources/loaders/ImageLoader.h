#ifndef HK_IMAGE_LOADER_H
#define HK_IMAGE_LOADER_H

#include "hkstl/strings/hkstring.h"

namespace hk::loader {

struct ImageInfo {
    u32 width;
    u32 height;
    u32 channels;

    // If pixels provided, usercode is responsible for freeing an array
    void *pixels = nullptr;
};

ImageInfo load_image(const hk::string &path);

}

#endif // HK_IMAGE_LOADER_H
