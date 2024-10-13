#include "Material.h"

#include "renderer/vkwrappers/Pipeline.h"
#include "resources/AssetManager.h"

namespace hk {

void hk::RenderMaterial::build(VkRenderPass renderpass, u32 pushConstSize,
                         VkDescriptorSetLayout sceneDescriptorLayout,
                         VkFormat swapchainFormat, VkFormat depthFormat)
{
    // TODO: Make this dynamic
    // hk::DescriptorLayout::Builder layoutBuilder;
    // layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS);
    // layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS);
    // layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    // materialLayout = layoutBuilder.build().layout();

    hk::DescriptorLayout *materialLayoutHK =
        new hk::DescriptorLayout(hk::DescriptorLayout::Builder()
        .addBinding(0,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(1,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );
    materialLayout = materialLayoutHK->layout();

    VkDevice device = context()->device();

    PipelineBuilder builder;

    const std::string path = "..\\engine\\assets\\shaders\\";
    u32 hndlVertexShader = hk::assets()->load(path + "DefaultVS.hlsl");
    u32 hndlPixelShader = hk::assets()->load(path + "TexturePS.hlsl");

    builder.setShader(ShaderType::Vertex, assets()->getShader(hndlVertexShader).module);
    builder.setShader(ShaderType::Pixel,  assets()->getShader(hndlPixelShader).module);

    hk::vector<hk::Format> layout = {
        // position
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // normal
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // texture coordinates
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC2 | hk::Format::B32,
    };
    builder.setVertexLayout(sizeof(Vertex), layout);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setRasterizer(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT);
    builder.setMultisampling();
    builder.setColorBlend();
    builder.setDepthStencil();
    builder.setRenderInfo(swapchainFormat, depthFormat);

    builder.setPushConstants(pushConstSize);

    hk::vector<VkDescriptorSetLayout> layouts = {
        sceneDescriptorLayout,
        materialLayout
    };

    builder.setLayout(layouts);

    pipeline.pipeline = builder.build(device, renderpass).handle();
    hk::debug::setName(pipeline.pipeline, "Material Pipeline");

    pipeline.layout = builder.build(device, renderpass).layout();
    hk::debug::setName(pipeline.layout, "Material Pipeline Layout");

    Buffer::BufferDesc uniformDisc;
    uniformDisc.type = Buffer::Type::UNIFORM_BUFFER;
    uniformDisc.usage = Buffer::Usage::NONE;
    uniformDisc.property = Buffer::Property::CPU_ACESSIBLE;
    uniformDisc.size = 1; // FIX: temp, make depend on framebuffers size
    uniformDisc.stride = sizeof(MaterialConstants);

    buf.init(uniformDisc);

    MaterialConstants cons;
    cons.color = 1.f;
    cons.metal = 1.f;
    buf.update(&cons);
}

MaterialInstance RenderMaterial::write(DescriptorAllocator &allocator, VkSampler sampler)
{
    MaterialInstance matData;
    matData.pipeline = &pipeline;

    matData.materialSet = allocator.allocate(materialLayout);

    writer.clear();

    writer.writeBuffer(0, buf.buffer(),
                       sizeof(MaterialConstants),
                       0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    writer.writeImage(1, material->diffuse->view(), sampler,
                      material->diffuse->layout(),
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    writer.updateSet(matData.materialSet);

    return matData;
}

}

