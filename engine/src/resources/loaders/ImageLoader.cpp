#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_FAILURE_STRINGS
#include "vendor/stb/stb_image.h"

namespace hk::loader {

ImageInfo load_image(const hk::string &path)
{
    ImageInfo out;

    // b8 is_hdr = stbi_is_hdr(path.c_str());

    i32 width, height, channels;
    // void *pixels;
    stbi_uc *pixels;

    // if (is_hdr) {
    //     pixels = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
    // } else {
    //     pixels = stbi_load(path.c_str(), &width, &height, &channels, 0);
    // }

    pixels = stbi_load(path.c_str(),
                       &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        LOG_ERROR("Failed to load texture:", path);
        return out;
    }

    // hk::Format format;
    // if (channels == 1) { format = hk::Format::R8_SRGB; }
    // else if (channels == 2) { format = hk::Format::R8G8_SRGB; }
    // else if (channels == 3) { format = hk::Format::R8G8B8_SRGB; }
    // else if (channels == 4) { format = hk::Format::R8G8B8A8_SRGB; }

    out = {
        static_cast<u32>(width),
        static_cast<u32>(height),
        static_cast<u32>(channels),

        pixels
    };

    return out;

    // TODO: make unload_image or just call free on pixels later yourself
    stbi_image_free(pixels);
}

}

