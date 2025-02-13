#include "ImageLoader.h"

#include "debug.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"

namespace hk::loader {

Image* loadImage(const std::string &path)
{
    i32 width, height, channels;
    stbi_uc *pixels = stbi_load(path.c_str(),
                                &width, &height, &channels,
                                STBI_rgb_alpha);

    if (!pixels) {
        LOG_ERROR("Failed to load texture:", path);
        return nullptr;
    }

    Image *image = new Image();
    image->init({
        hk::Image::Usage::TRANSFER_DST | hk::Image::Usage::SAMPLED,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        static_cast<u32>(width), static_cast<u32>(height),
        static_cast<u32>(channels)
    });
    image->write(pixels);

    stbi_image_free(pixels);

    return image;
}

}

