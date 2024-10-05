#ifndef HK_MATERIAL_H
#define HK_MATERIAL_H

#include "renderer/vkwrappers/Image.h"
#include "renderer/Descriptors.h"

#include "utils/containers/hkvector.h"

#include "math/vec4f.h"
#include "renderer/vkwrappers/Buffer.h"

namespace hk {

struct Material {
    // TODO: change to handles
    hk::Image *diffuse;
    // hk::Image *normal;
    // hk::Image *emissive;
    // hk::Image *metalness;
    // hk::Image *roughness;
    // hk::Image *lightmap; // Ambient Occlusion

    u32 hndlVertexShader;
    u32 hndlPixelShader;
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

    struct MaterialConstants {
        hkm::vec4f color;
        hkm::vec4f metal;
        //padding, we need it anyway for uniform buffers
        // hkm::vec4f extra[14];
    };

    Buffer buf;
    Material *material;

    // struct MaterialResources {
    //     hk::Image *diffuse;
    //     // VkSampler colorSampler;
    //     // AllocatedImage metalRoughImage;
    //     // VkSampler metalRoughSampler;
    //
    //     VkBuffer dataBuffer;
    //     u32 dataBufferOffset;
    // };
    // const MaterialResources resources;


    void build(VkRenderPass renderpass, u32 pushConstSize,
               VkDescriptorSetLayout sceneDescriptorLayout,
               VkFormat swapchainFormat, VkFormat depthFormat);
    void clear();

    MaterialInstance write(DescriptorAllocator &allocator, VkSampler sampler);
};

}

#endif // HK_MATERIAL_H
