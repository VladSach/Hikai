#ifndef HK_TEXTURE_LOADER_H
#define HK_TEXTURE_LOADER_H

#include "defines.h"
#include "renderer/Texture.h"

namespace hk::loader {

HKAPI Texture* loadTexture(const std::string &path);

}

#endif // HK_MODEL_LOADER_H
