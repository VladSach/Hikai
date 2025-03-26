#include "Material.h"

#include "renderer/vkwrappers/vkdebug.h"

#include "resources/AssetManager.h"

namespace hk {

void hk::RenderMaterial::build(VkRenderPass renderpass, u32 pushConstSize,
                         VkDescriptorSetLayout sceneDescriptorLayout,
                         VkDescriptorSetLayout passDescriptorLayout,
                         hk::vector<VkFormat> formats, VkFormat depthFormat,
                         const std::string &name)
{
    // TODO: Make this dynamic
    hk::DescriptorLayout::Builder l_builder;
    l_builder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    l_builder.addBinding(6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  VK_SHADER_STAGE_FRAGMENT_BIT);
    layout.init(l_builder.build());

    PipelineBuilder builder;

    builder.setShader(material->vertex_shader);
    builder.setShader(material->pixel_shader);

    hk::vector<Format> vert_layout = {
        // position
        hk::Format::R32G32B32_SFLOAT,

        // normal
        hk::Format::R32G32B32_SFLOAT,

        // texture coordinates
        hk::Format::R32G32_SFLOAT,

        // tangent
        hk::Format::R32G32B32_SFLOAT,

        // hk::Format::SIGNED | hk::Format::FLOAT |
        // hk::Format::VEC3 | hk::Format::B32,
    };
    builder.setVertexLayout(sizeof(Vertex), vert_layout);

    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setRasterizer(VK_POLYGON_MODE_FILL,
                          material->twosided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT,
                          VK_FRONT_FACE_CLOCKWISE);
    builder.setMultisampling();

    builder.setDepthStencil(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, depthFormat);
    // FIX: temp
    hk::vector<std::pair<VkFormat, BlendState>> colors = {
        { VK_FORMAT_R32G32B32A32_SFLOAT, hk::BlendState::DEFAULT },
        { VK_FORMAT_R16G16B16A16_SFLOAT, hk::BlendState::NONE },
        { VK_FORMAT_R8G8B8A8_UNORM,      hk::BlendState::NONE },
        { VK_FORMAT_R8G8B8A8_UNORM,      hk::BlendState::NONE },
        { VK_FORMAT_R32_SFLOAT,          hk::BlendState::NONE },
    };
    builder.setColors(colors);

    builder.setPushConstants({{VK_SHADER_STAGE_ALL_GRAPHICS, 0, pushConstSize }});

    hk::vector<VkDescriptorSetLayout> layouts = {
        sceneDescriptorLayout,
        passDescriptorLayout,
        layout.handle(),
    };

    builder.setDescriptors(layouts);

    hk::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    builder.setDynamicStates(dynamic_states);

    pipeline = builder.build(renderpass);
    // hk::debug::setName(pipeline.handle(), "Material Pipeline");
    // hk::debug::setName(pipeline.layout(), "Material Pipeline Layout");

    BufferDesc uniform_desc;
    uniform_desc.type = BufferType::UNIFORM_BUFFER;
    uniform_desc.access = MemoryType::CPU_UPLOAD;
    uniform_desc.size = 1; // FIX: temp, make depend on framebuffers size
    uniform_desc.stride = sizeof(Material::constants);

    buf = bkr::create_buffer(uniform_desc, name);
}

MaterialInstance RenderMaterial::write(DescriptorAllocator &allocator, VkSampler sampler)
{
    MaterialInstance matData;
    matData.pipeline = &pipeline;

    matData.materialSet = allocator.allocate(layout.handle());
    static u32 count = 0;
    hk::debug::setName(matData.materialSet,
                       "Descriptor Set - Material #" + std::to_string(count++));

    writer.clear();

    bkr::update_buffer(buf, &material->constants);
    writer.writeBuffer(0, bkr::handle(buf),
                       sizeof(Material::constants), 0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    hk::ImageHandle map;
    for (u32 i = 0; i < Material::MAX_TEXTURE_TYPE; ++i) {
        u32 handle = material->map_handles[i];

        map = hk::assets()->getTexture(handle).image;
        writer.writeImage(i + 1, hk::bkr::view(map), sampler,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          // TODO: If sampler != nullptr -> use combined image
                          VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    }

    writer.updateSet(matData.materialSet);

    return matData;
}

void RenderMaterial::clear()
{
    bkr::destroy_buffer(buf);
    pipeline.deinit();
    layout.deinit();
    material = nullptr;
}

}

