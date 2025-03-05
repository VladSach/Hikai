#include "Material.h"

#include "renderer/vkwrappers/vkdebug.h"

#include "resources/AssetManager.h"

namespace hk {

void hk::RenderMaterial::build(VkRenderPass renderpass, u32 pushConstSize,
                         VkDescriptorSetLayout sceneDescriptorLayout,
                         VkDescriptorSetLayout passDescriptorLayout,
                         hk::vector<VkFormat> formats, VkFormat depthFormat)
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
        .addBinding(2,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(3,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(4,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(5,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .addBinding(6,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_ALL_GRAPHICS)
        .build()
    );
    materialLayout = materialLayoutHK->layout();

    VkDevice device = context()->device();

    PipelineBuilder builder;

    builder.setShader(ShaderType::Vertex, assets()->getShader(material->vertex_shader).module);
    builder.setShader(ShaderType::Pixel,  assets()->getShader(material->pixel_shader).module);

    hk::vector<hk::bitflag<Format>> layout = {
        // position
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // normal
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,

        // texture coordinates
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC2 | hk::Format::B32,

        // tangent
        hk::Format::SIGNED | hk::Format::FLOAT |
        hk::Format::VEC3 | hk::Format::B32,
    };
    builder.setVertexLayout(sizeof(Vertex), layout);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          material->twosided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT,
                          VK_FRONT_FACE_CLOCKWISE);
    builder.setMultisampling();
    builder.setColorBlend();
    builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    builder.setRenderInfo(formats, depthFormat);

    builder.setPushConstants(pushConstSize);

    hk::vector<VkDescriptorSetLayout> layouts = {
        sceneDescriptorLayout,
        passDescriptorLayout,
        materialLayout
    };

    builder.setLayout(layouts);

    pipeline = builder.build(device, renderpass);
    // hk::debug::setName(pipeline.handle(), "Material Pipeline");
    // hk::debug::setName(pipeline.layout(), "Material Pipeline Layout");

    Buffer::BufferDesc uniformDisc;
    uniformDisc.type = Buffer::Type::UNIFORM_BUFFER;
    uniformDisc.usage = Buffer::Usage::NONE;
    uniformDisc.property = Buffer::Property::CPU_ACESSIBLE;
    uniformDisc.size = 1; // FIX: temp, make depend on framebuffers size
    uniformDisc.stride = sizeof(Material::constants);

    buf.init(uniformDisc);
}

MaterialInstance RenderMaterial::write(DescriptorAllocator &allocator, VkSampler sampler)
{
    MaterialInstance matData;
    matData.pipeline = &pipeline;

    matData.materialSet = allocator.allocate(materialLayout);

    writer.clear();

    buf.update(&material->constants);
    writer.writeBuffer(0, buf.buffer(),
                       sizeof(Material::constants), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    hk::Image *map;
    for (u32 i = 0; i < Material::MAX_TEXTURE_TYPE; ++i) {
        u32 handle = material->map_handles[i];

        map = hk::assets()->getTexture(handle).texture;
        writer.writeImage(i + 1, map->view(), sampler, map->layout(),
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    writer.updateSet(matData.materialSet);

    return matData;
}

void RenderMaterial::clear()
{
    buf.deinit();
    material = nullptr;
}

}

