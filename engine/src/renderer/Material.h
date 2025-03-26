#ifndef HK_MATERIAL_H
#define HK_MATERIAL_H

#include "math/vec4f.h"

#include "hkstl/containers/hkvector.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Descriptors.h"

#include "renderer/resources.h"

namespace hk {

struct Material {
    struct {
        hkm::vec4f color = 0.f;
        hkm::vec4f specular = 0.f;
        hkm::vec4f ambient = 0.f;

        f32 opacity = 1.f;

        // f32 shininess = 0.f;
        // f32 reflectivity = 0.f;
        f32 metalness = 0.f;
        f32 roughness = 0.f;
    } constants;

    b8 twosided = false;

    enum TextureType {
        BASECOLOR = 0,
        NORMAL,
        EMISSIVE,
        METALNESS,
        ROUGHNESS,
        AMBIENT_OCCLUSION,

        MAX_TEXTURE_TYPE
    };

    u32 map_handles[MAX_TEXTURE_TYPE];

    // Shaders
    u32 vertex_shader;
    u32 pixel_shader;
};

struct MaterialInstance {
    hk::Pipeline *pipeline;
    VkDescriptorSet materialSet;
};

struct RenderMaterial {
    // ~RenderMaterial() { clear(); }

    hk::Pipeline pipeline;

    hk::DescriptorWriter writer;
    hk::DescriptorLayout layout;

    hk::BufferHandle buf;
    Material *material;

    void build(VkRenderPass renderpass, u32 pushConstSize,
               VkDescriptorSetLayout sceneDescriptorLayout,
               VkDescriptorSetLayout passDescriptorLayout,
               hk::vector<VkFormat> formats, VkFormat depthFormat,
               const std::string &name);

    void clear();

    MaterialInstance write(DescriptorAllocator &allocator,
                           VkSampler sampler = VK_NULL_HANDLE);
};

}

#endif // HK_MATERIAL_H
