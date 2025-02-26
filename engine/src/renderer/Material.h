#ifndef HK_MATERIAL_H
#define HK_MATERIAL_H

#include "math/vec4f.h"

#include "utils/containers/hkvector.h"

#include "renderer/vkwrappers/Buffer.h"
#include "renderer/vkwrappers/Image.h"
#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Descriptors.h"

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
    hk::Pipeline pipeline;

    DescriptorWriter writer;
    VkDescriptorSetLayout materialLayout;

    Buffer buf;
    Material *material;

    void build(VkRenderPass renderpass, u32 pushConstSize,
               VkDescriptorSetLayout sceneDescriptorLayout,
               VkFormat swapchainFormat, VkFormat depthFormat);
    void clear();

    MaterialInstance write(DescriptorAllocator &allocator, VkSampler sampler);
};

}

#endif // HK_MATERIAL_H
