#ifndef HK_IMAGE_LOADER_H
#define HK_IMAGE_LOADER_H

#include "defines.h"
#include "renderer/vkwrappers/Image.h"

namespace hk::loader {

Image* loadImage(const std::string &path);

}

#endif // HK_IMAGE_LOADER_H
