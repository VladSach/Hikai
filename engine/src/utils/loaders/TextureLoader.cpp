#include "TextureLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"

#include <unordered_map>

namespace hk::loader {

static std::unordered_map<std::string, Texture*> loaded_ = {};

Texture* loadTexture(const std::string &path)
{
    i32 width, height, channels;
    stbi_uc *pixels = stbi_load(path.c_str(),
                                &width, &height, &channels,
                                STBI_rgb_alpha);

    ALWAYS_ASSERT(pixels, "Failed to load texture: ", path);

    Texture *texture = new Texture(pixels, width, height, channels);

    loaded_[path] = texture;

    return texture;
}

}

