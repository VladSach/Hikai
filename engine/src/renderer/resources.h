#ifndef HK_RESOURCES_H
#define HK_RESOURCES_H

#include "hkstl/utility/hktypes.h"
#include "hkstl/strings/hkstring.h"

#include "resource_types.h"

// FIX: temp
#include "vulkan/vulkan.h"

// Back-end resources
namespace hk::bkr {

void init();
void deinit();

/* ===== Buffers ===== */
BufferHandle create_buffer(const BufferDesc &desc, const hk::string &name = "");
void destroy_buffer(const BufferHandle &handle);

void resize_buffer(const BufferHandle &handle, u32 size);
void update_buffer(const BufferHandle &handle, const void *data);
void bind_buffer(const BufferHandle &handle, VkCommandBuffer cmd);

const BufferDesc& desc(const BufferHandle &handle);
const ResourceMetadata& meta(const BufferHandle &handle);

// FIX: temp
VkBuffer handle(BufferHandle handle);
HKAPI const hk::vector<ResourceMetadata>& metadata();
HKAPI const hk::vector<BufferDesc>& descriptors();
HKAPI const u32 size();

/* ===== Images ===== */
ImageHandle create_image(const ImageDesc &desc, const std::string &name = "");
void destroy_image(const ImageHandle &handle);

void write_image(const ImageHandle &handle, const void *pixels);
void copy_image(const ImageHandle &src, const ImageHandle &dst);
void transition_image_layout(const ImageHandle &handle, VkImageLayout target);

const ImageDesc& desc(ImageHandle handle);
const ResourceMetadata& meta(ImageHandle handle);

// FIX: temp
VkImage image(ImageHandle handle);
VkImageView view(ImageHandle handle);
HKAPI const hk::vector<ResourceMetadata>& image_metadata();
HKAPI const hk::vector<ImageDesc>& image_descriptors();
HKAPI const u32 image_size();

/* ===== Shaders ===== */
// struct ShaderMeta {
//     std::string name;
//     std::string code;
//     hk::dxc::ShaderDesc desc;
// };
// struct Shader {
//     ShaderMeta meta;
//
//     VkShaderModule module;
// };
// u32 loadShader(hk::dxc::ShaderDesc desc);
// void unloadShader(u32 handle);
// void reloadShader(u32 handle);
//
// ShaderMeta getMetadata(u32 handle);
// void attachToPipeline(u32 handle/*, pipeline*/);

}

#endif // HK_RESOURCES_H
