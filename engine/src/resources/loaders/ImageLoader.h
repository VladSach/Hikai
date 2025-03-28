#ifndef HK_IMAGE_LOADER_H
#define HK_IMAGE_LOADER_H

#include "renderer/vkwrappers/Image.h"

#include <string>

namespace hk::loader {

Image* loadImage(const std::string &path);

}

#endif // HK_IMAGE_LOADER_H
