#include "Pipeline.h"

#include "renderer/VulkanContext.h"

namespace hk {

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

void PipelineBuilder::setShader(ShaderType type, VkShaderModule shader)
{
    VkShaderStageFlagBits stage;

    switch(type) {
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

    VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    shaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shader;
    shaderStageInfo.pName = "main";

    shaderStages.push_back(shaderStageInfo);
}

void PipelineBuilder::setVertexLayout(u32 stride,
                                      const hk::vector<hk::Format> &layout)
{
    bindingDescription.binding = 0;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    if (!stride) {
        bindingDescription.stride = 0;
        vertexInputInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
        return;
    }

    bindingDescription.stride = stride;

    attributeDescs = hk::createVertexLayout(layout);

    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.data();
}

void PipelineBuilder::setInputTopology(VkPrimitiveTopology topology)
{
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::setRasterizer(VkPolygonMode polygonMode, VkCullModeFlags cullMode)
{
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
}

void PipelineBuilder::setMultisampling()
{
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::setColorBlend()
{
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::setDepthStencil()
{
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional
}

void PipelineBuilder::setLayout(const hk::vector<VkDescriptorSetLayout> &layouts)
{
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
}

void PipelineBuilder::setPushConstants(u32 structSize)
{
    pushConstant.offset = 0;
    pushConstant.size = structSize;
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // TODO: configurable

    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
}

void PipelineBuilder::setRenderInfo(VkFormat colorFormat, VkFormat depthFormat)
{
    colorAttachmentformat = colorFormat;
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentformat;
    renderInfo.depthAttachmentFormat = depthFormat;
}

hk::Pipeline PipelineBuilder::build(VkDevice device, VkRenderPass pass)
{
    VkResult err;

    Pipeline out;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    hk::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    err = vkCreatePipelineLayout(device, &pipelineLayoutInfo,
                                 nullptr, &out.layout());
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Pipeline Layout");

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderInfo;

    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages    = shaderStages.data();

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.pDepthStencilState  = &depthStencil;

    pipelineInfo.layout = out.layout();
    pipelineInfo.renderPass = pass;
    pipelineInfo.subpass = 0;

    err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                    &pipelineInfo, nullptr, &out.handle());
    ALWAYS_ASSERT(!err, "Failed to create Vulkan Graphics Pipeline");

    return out;
}

void PipelineBuilder::clear()
{
    shaderStages.clear();

    vertexInputInfo = {};
    inputAssembly = {};
    rasterizer = {};
    multisampling = {};
    colorBlendAttachment = {};
    depthStencil = {};
    pipelineLayoutInfo = {};
    renderInfo = {};
    colorAttachmentformat = VK_FORMAT_UNDEFINED;

    attributeDescs.clear();
    bindingDescription = {};
}

}
