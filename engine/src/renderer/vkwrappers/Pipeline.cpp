#include "Pipeline.h"

#include "renderer/VulkanContext.h"
#include "renderer/vkwrappers/vkdebug.h"

#include "resources/AssetManager.h"

namespace hk {

/* ================================ Pipeline ================================ */
void Pipeline::init(VkPipeline pipeline, VkPipelineLayout layout)
{
    handle_ = pipeline;
    layout_ = layout;
}

void Pipeline::deinit()
{
    VkDevice device = hk::context()->device();

    vkDestroyPipeline(device, handle_, nullptr);
    vkDestroyPipelineLayout(device, layout_, nullptr);
}

void Pipeline::bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point)
{
    vkCmdBindPipeline(cmd, bind_point, handle_);
}

/* ============================ Pipeline Builder ============================ */
void PipelineBuilder::setName(const std::string &name)
{
    out_.info_.name = name;
}

void PipelineBuilder::setDescriptors(
    const hk::vector<VkDescriptorSetLayout> &layouts)
{
    desc_layouts_ = layouts;
    layout_info_.pSetLayouts = desc_layouts_.data();
    layout_info_.setLayoutCount = desc_layouts_.size();
}

void PipelineBuilder::setPushConstants(
    const hk::vector<VkPushConstantRange> &ranges)
{
    u32 curr_size = 0;
    for (auto range : ranges) {
        curr_size += range.size;
    }
    // 128 bytes is minimum by chapter 30.2.1 of the specs
    CHECK_DEVICE_LIMIT(<=, curr_size, limits.maxPushConstantsSize);

    push_ranges_ = ranges;

    layout_info_.pPushConstantRanges = push_ranges_.data();
    layout_info_.pushConstantRangeCount = push_ranges_.size();
}

void PipelineBuilder::setShader(u32 hndl_shader)
{
    VkShaderStageFlagBits stage;

    hk::ShaderAsset &shader = hk::assets()->getShader(hndl_shader);

    switch(shader.desc.type) {
    case ShaderType::Vertex: {
        stage = VK_SHADER_STAGE_VERTEX_BIT;
    } break;
    case ShaderType::Pixel: {
        stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    } break;
    default:
        stage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        LOG_ERROR("Unspecified shader stage");
    }

    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = stage; // value specifying a SINGLE pipeline stage
    stage_info.module = shader.module;
    stage_info.pName = shader.desc.entry.c_str();

    u32 type = static_cast<u32>(shader.desc.type);
    if (out_.info_.shaders.count(type)) {
        LOG_WARN("Can't overwrite shaders in pipeline");
        return;
    }

    shader_stages_.push_back(stage_info);

    out_.info_.shaders[type] = hndl_shader;
}

void PipelineBuilder::setVertexLayout(
    u32 stride, const hk::VertexLayout &layout)
{
    vertex_binding_.binding = 0;
    vertex_binding_.stride = stride;
    vertex_binding_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    if (!stride) {
        vertex_input_.vertexBindingDescriptionCount = 0;
        vertex_input_.pVertexBindingDescriptions = nullptr;
        vertex_input_.vertexAttributeDescriptionCount = 0;
        vertex_input_.pVertexAttributeDescriptions = nullptr;
        return;
    }

    // NOTE: Limits checked in hk::createVertexLayout
    attribute_descs_ = hk::createVertexLayout(layout);

    vertex_input_.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_.vertexBindingDescriptionCount = 1;
    vertex_input_.pVertexBindingDescriptions = &vertex_binding_;
    vertex_input_.vertexAttributeDescriptionCount = attribute_descs_.size();
    vertex_input_.pVertexAttributeDescriptions = attribute_descs_.data();

    out_.info_.vertex_layout = layout;
}

void PipelineBuilder::setInputTopology(VkPrimitiveTopology topology)
{
    input_assembly_.topology = topology;
    input_assembly_.primitiveRestartEnable = VK_FALSE;

    out_.info_.topology = topology;
}

void PipelineBuilder::setTessellation(u32 points_per_patch)
{
    if (points_per_patch <= 0) { return; }
    CHECK_DEVICE_LIMIT(<=, points_per_patch, limits.maxTessellationPatchSize);

    tessellation_.patchControlPoints = points_per_patch;
}

void PipelineBuilder::setViewports(
    const hk::vector<VkViewport> &viewports,
    const hk::vector<VkRect2D>   &scissors)
{
    ALWAYS_ASSERT(viewports.size() == scissors.size(),
                  "Number of viewports and scissors must match");
    ALWAYS_ASSERT((features.multiViewport || !(viewports.size() > 1)),
                  "Multiple viewports provided, but multiViewport feature "
                  "is not enabled");
    CHECK_DEVICE_LIMIT(<=, viewports.size(), limits.maxViewports);

    viewports_ = viewports;
    scissors_ = scissors;

    viewport_.viewportCount = viewports_.size();
    viewport_.pViewports = viewports_.data();
    viewport_.scissorCount = scissors_.size();
    viewport_.pScissors = scissors_.data();
}

void PipelineBuilder::setRasterizer(
    VkPolygonMode polygon_mode,
    VkCullModeFlags cull_mode,
    VkFrontFace front_face)
{
    // ALWAYS_ASSERT((!features.depthClamp && enable_depth_clamp),
    //               "Depth clamp is requested, but depthClamp feature "
    //               "is not enabled");
    ALWAYS_ASSERT((features.fillModeNonSolid ||
                  (polygon_mode == VK_POLYGON_MODE_FILL)),
                  "Polygon Mode is not Fill, but fillModeNonSolid feature "
                  "is not enabled");

    rasterizer_.depthClampEnable = VK_FALSE;
    rasterizer_.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_.polygonMode = polygon_mode;
    rasterizer_.cullMode = cull_mode;
    rasterizer_.frontFace = front_face;
    rasterizer_.depthBiasEnable = VK_FALSE;
    rasterizer_.lineWidth = 1.0f;
}

void PipelineBuilder::setMultisampling()
{
    // ALWAYS_ASSERT((!features.alphaToOne && enable_alpha_to_one),
    //               "Alpha to One is requested, but alphaToOne feature "
    //               "is not enabled");

    multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_.sampleShadingEnable = VK_FALSE;
    // multisampling.minSampleShading
    // multisampling.pSampleMask
    multisampling_.alphaToCoverageEnable = VK_FALSE;
    multisampling_.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::setDepthStencil(
    VkBool32 enable,
    VkCompareOp op,
    VkFormat format)
{
    depth_stencil_.depthTestEnable = enable;
    depth_stencil_.depthWriteEnable = enable;
    depth_stencil_.depthCompareOp = op;
    depth_stencil_.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_.stencilTestEnable = VK_FALSE;
    // depth_stencil.front
    // depth_stencil.back
    depth_stencil_.minDepthBounds = 0.0f;
    depth_stencil_.maxDepthBounds = 1.0f;

    rendering_info_.depthAttachmentFormat = format;
}

void PipelineBuilder::setColors(
    const hk::vector<std::pair<VkFormat, BlendState>> &atts)
{
    colors_.reserve(atts.size());
    blend_states_.reserve(atts.size());

    // TODO: Check for support of independent blend

    VkPipelineColorBlendAttachmentState blend = {};
    for (auto att : atts) {
        colors_.push_back(att.first);

        blend.blendEnable =
            (att.second == BlendState::NONE) ? VK_FALSE : VK_TRUE;

        blend.colorWriteMask  = VK_COLOR_COMPONENT_R_BIT;
        blend.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
        blend.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
        blend.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

        // TODO: switch-case with all BlendStates
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;

        blend_states_.push_back(blend);
    }

    color_blending_.logicOpEnable = VK_FALSE;
    color_blending_.logicOp = VK_LOGIC_OP_COPY;
    color_blending_.attachmentCount = blend_states_.size();
    color_blending_.pAttachments = blend_states_.data();

    rendering_info_.colorAttachmentCount = colors_.size();
    rendering_info_.pColorAttachmentFormats = colors_.data();

    out_.info_.attachments = atts;
}

void PipelineBuilder::setDynamicStates(const hk::vector<VkDynamicState> &states)
{
    for (auto state : states) {
        switch (state) {
        case VK_DYNAMIC_STATE_VIEWPORT: { viewport_.viewportCount = 1; } break;
        case VK_DYNAMIC_STATE_SCISSOR:  { viewport_.scissorCount = 1;  } break;

        default:
            break;
        }
    }

    dynamic_states_ = states;

    dynamic_state_.dynamicStateCount = dynamic_states_.size();
    dynamic_state_.pDynamicStates = dynamic_states_.data();
}

hk::Pipeline PipelineBuilder::build(VkRenderPass pass, u32 subpass)
{
    VkResult err;

    err = vkCreatePipelineLayout(device, &layout_info_, nullptr, &out_.layout_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Pipeline Layout");

    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &rendering_info_;

    // info.flags = flags_;

    info.stageCount = shader_stages_.size();
    info.pStages    = shader_stages_.data();

    info.pVertexInputState   = &vertex_input_;
    info.pInputAssemblyState = &input_assembly_;
    info.pTessellationState  = &tessellation_;
    info.pViewportState      = &viewport_;
    info.pRasterizationState = &rasterizer_;
    info.pMultisampleState   = &multisampling_;
    info.pDepthStencilState  = &depth_stencil_;
    info.pColorBlendState    = &color_blending_;
    info.pDynamicState       = &dynamic_state_;

    info.layout = out_.layout_;
    info.renderPass = pass;
    info.subpass = subpass;

    err = vkCreateGraphicsPipelines(device, out_.cache_, 1,
                                    &info, nullptr, &out_.handle_);
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Graphics Pipeline");

    hk::debug::setName(out_.handle_, "Pipeline - " + out_.info_.name);
    hk::debug::setName(out_.layout_, "Pipeline Layout - " + out_.info_.name);
    out_.info_.push_ranges = push_ranges_;
    out_.info_.desc_layouts = desc_layouts_;
    out_.info_.viewports = viewports_;
    out_.info_.dynamic_states = dynamic_states_;

    return out_;
}

void PipelineBuilder::clear()
{
    /* ===== Pipeline Layout ===== */
    layout_info_ = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    /* ===== Pipeline Flags ===== */
    flags_ = 0;

    /* ===== Pipeline States ===== */
    shader_stages_.clear();
    vertex_input_ =
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    input_assembly_ =
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    tessellation_ =
        { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
    viewport_ =
        { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    rasterizer_ =
        { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    multisampling_ =
        { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    depth_stencil_ =
        { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    color_blending_ =
        { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    dynamic_state_ =
        { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

    /* ===== Pipeline Layout Data ===== */
    push_ranges_.clear();
    desc_layouts_.clear();

    /* ===== Vertex State Data ===== */
    vertex_binding_ = {};
    attribute_descs_.clear();

    /* ===== Viewport State Data ===== */
    viewports_.clear();
    scissors_.clear();

    /* ===== Color Blending State Data ===== */
    blend_states_.clear();

    /* ===== Dynamic State Data ===== */
    dynamic_states_.clear();

    /* ===== Pipeline Extending Stages ===== */
    colors_.clear();
    rendering_info_ = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

    /* ===== Hikai Pipeline Wrapper ===== */
    out_ = hk::Pipeline();

    // Convenience
    device = hk::context()->device();
    limits = hk::context()->physicalInfo().properties.limits;
    features = hk::context()->physicalInfo().features;
}

}
