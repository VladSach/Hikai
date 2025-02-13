#ifndef HK_MATERIAL_H
#define HK_MATERIAL_H

#include "math/vec4f.h"

#include "utils/containers/hkvector.h"

#include "renderer/vkwrappers/Buffer.h"
#include "renderer/vkwrappers/Image.h"
#include "renderer/vkwrappers/Descriptors.h"

namespace hk {

struct Material {
    struct {
        hkm::vec4f color = 1.f;
        hkm::vec4f emissive;
        f32 alpha = 1.f;
        f32 shininess = 0.f;
        f32 reflectivity = 0.f;
    } constants;

    u32 hndlDiffuse = 0;
    u32 hndlNormal = 0;
    // hk::Image *diffuse;
    // hk::Image *normal;
    // hk::Image *emissive;
    // hk::Image *metalness;
    // hk::Image *roughness;
    // hk::Image *lightmap; // Ambient Occlusion

    // Shanders
    u32 hndlVS;
    u32 hndlPS;
};


struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
};

struct RenderMaterial {
    MaterialPipeline pipeline;
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
