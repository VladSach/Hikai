#ifndef HK_ASSET_H
#define HK_ASSET_H

#include "defines.h"
#include "utils/containers/hkvector.h"

#include "vendor/vulkan/vulkan.h"
#include "renderer/debug.h"
#include "renderer/VulkanContext.h"
#include "loaders/ShaderLoader.h"

#include "resources/loaders/ImageLoader.h"

#include "renderer/object/Mesh.h"
#include "renderer/Material.h"

namespace hk {

struct Asset {
    // 26 bits for index + 6 unique bits
    // OR
    // 20 bits for index + 6 bits for type + 6 unique bits
    u32 handle = 0xdeadcell;

    std::string name = "Undefined";
    std::string path = "Void";

    enum class Type : u32 {
        NONE = 0,

        TEXTURE,
        SHADER,
        MESH,
        MATERIAL,
        MODEL,

        MAX_ASSET_TYPE,
    } type;

    virtual ~Asset() {};
};

// TODO: rename
constexpr u32 getIndex(u32 handle)
{
    return handle & ((1 << 26) - 1);
}

constexpr u32 getUnique(u32 handle)
{
    return (handle & ~((1 << 26) - 1)) >> 26;
}

struct ShaderAsset : public Asset {
    hk::dxc::ShaderDesc desc;
    hk::vector<u32> code;
    VkShaderModule module;

    ~ShaderAsset() { deinit(); }

    void createShaderModule()
    {
        VkResult err;
        VkDevice device = hk::context()->device();

        VkShaderModuleCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size() * sizeof(u32);
        info.pCode = code.data();

        err = vkCreateShaderModule(device, &info, nullptr, &module);
        ALWAYS_ASSERT(!err, "Failed to create Vulkan Shader Module");

        hk::debug::setName(module, name);
    }

    void deinit()
    {
        code.clear();
        vkDestroyShaderModule(hk::context()->device(), module, nullptr);
    }
};

struct TextureAsset : public Asset {
    hk::Image *texture;
};

struct MaterialAsset : public Asset {
    hk::Material *material;
};

struct MeshAsset : public Asset {
    Mesh mesh;

    u32 cntInstances = 1;

    hk::vector<MeshAsset*> children;
    hk::vector<hkm::mat4f> instances;
    hk::vector<hkm::mat4f> instancesInv;

    hk::vector<u32> hndlTextures;
};

struct ModelAsset : public Asset {
    u32 hndlRootMesh;
};


}

#endif // HK_ASSET_H
