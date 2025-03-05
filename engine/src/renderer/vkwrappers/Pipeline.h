#ifndef HK_PIPELINE_H
#define HK_PIPELINE_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/VertexLayout.h"

#include "utils/containers/hkvector.h"

#include "resources/loaders/ShaderLoader.h"

namespace hk {

class Pipeline {
public:
    void init(VkPipeline pipeline, VkPipelineLayout layout);
    void deinit();

public:
    constexpr VkPipeline &handle() { return handle_; }
    constexpr VkPipelineLayout &layout() { return layout_; }

private:
    VkPipeline handle_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
};

class PipelineBuilder {
public:
    PipelineBuilder() { clear(); }

    void clear();

    void setShader(ShaderType type, VkShaderModule shader);
    void setVertexLayout(u32 stride,
                         const hk::vector<hk::bitflag<hk::Format>> &layout);
    void setInputTopology(VkPrimitiveTopology topology);
    void setRasterizer(VkPolygonMode polygonMode,
                       VkCullModeFlags cullMode,
                       VkFrontFace front) ;
    void setMultisampling();
    void setColorBlend();
    void setDepthStencil(VkBool32 enable, VkCompareOp op);
    void setLayout(const hk::vector<VkDescriptorSetLayout> &layouts);
    void setPushConstants(u32 structSize);
    void setRenderInfo(const hk::vector<VkFormat> &colorFormats,
                       VkFormat depthFormat);

    hk::Pipeline build(VkDevice device, VkRenderPass pass, u32 subpass = 0);

private:
    hk::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineVertexInputStateCreateInfo   vertexInputInfo = {};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo   multisampling = {};
    VkPipelineColorBlendAttachmentState    colorBlendAttachment = {};
    VkPipelineDepthStencilStateCreateInfo  depthStencil = {};
    VkPipelineLayoutCreateInfo             pipelineLayoutInfo = {};

    hk::vector<VkVertexInputAttributeDescription> attributeDescs;
    VkVertexInputBindingDescription bindingDescription = {};

    VkPipelineRenderingCreateInfo renderInfo = {};
    hk::vector<VkFormat> colorAttachmentFormats;

    // FIX: temp
    VkPushConstantRange pushConstant = {};
};

}

#endif // HK_PIPELINE_H
