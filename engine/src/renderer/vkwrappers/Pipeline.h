#ifndef HK_PIPELINE_H
#define HK_PIPELINE_H

#include "vendor/vulkan/vulkan.h"

#include "renderer/resources.h"

#include "hkstl/containers/hkvector.h"

namespace hk {

using VertexLayout = hk::vector<hk::Format>;

enum class BlendState {
    NONE = 0,
    DEFAULT = 1,
    MAX_STATES
};

class Pipeline {
public:
    void init(VkPipeline pipeline, VkPipelineLayout layout);
    void deinit();

    void bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point);

public:
    constexpr VkPipeline handle() const
    {
        DEV_ASSERT(handle_, "Trying to access VK_NULL_HANDLE");
        return handle_;
    }

    constexpr VkPipelineLayout layout() const
    {
        DEV_ASSERT(handle_, "Trying to access VK_NULL_HANDLE");
        return layout_;
    }

private:
    VkPipeline handle_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;

    VkPipelineCache cache_ = VK_NULL_HANDLE;

    friend class PipelineBuilder;

public:
    struct Info {
        std::string name = "Unknown";

        /* ===== Pipeline Layout Info ===== */
        hk::vector<VkDescriptorSetLayout> desc_layouts;
        hk::vector<VkPushConstantRange> push_ranges;

        /* ===== Pipeline Info ===== */
        std::unordered_map<u32, u32> shaders; // ShaderType, handle
        hk::VertexLayout vertex_layout;
        VkPrimitiveTopology topology;
        hk::vector<VkViewport> viewports;

        hk::vector<std::pair<VkFormat, BlendState>> attachments;

        hk::vector<VkDynamicState> dynamic_states;
    } info_;
};

class PipelineBuilder {
public:
    PipelineBuilder() { clear(); }

    void clear();
    hk::Pipeline build(VkRenderPass pass, u32 subpass = 0);

    void setName(const std::string &name);

    /* ===== Pipeline Layout Info ===== */
    void setDescriptors(const hk::vector<VkDescriptorSetLayout> &layouts);
    void setPushConstants(const hk::vector<VkPushConstantRange> &ranges);

    /* ===== Pipeline Info ===== */
    void setShader(u32 hndl_shader);
    void setVertexLayout(u32 stride, const hk::VertexLayout &layout);
    void setInputTopology(VkPrimitiveTopology topology);
    void setTessellation(u32 points_per_patch);
    void setViewports(const hk::vector<VkViewport> &viewports,
                      const hk::vector<VkRect2D>   &scissors);
    void setRasterizer(VkPolygonMode polygon_mode,
                       VkCullModeFlags cull_mode,
                       VkFrontFace front_face);
    void setMultisampling();
    void setDepthStencil(VkBool32 enable, VkCompareOp op, VkFormat format);
    void setColors(const hk::vector<std::pair<VkFormat, BlendState>> &atts);
    void setDynamicStates(const hk::vector<VkDynamicState> &states);

private:
    /* ===== Pipeline Layout ===== */
    VkPipelineLayoutCreateInfo layout_info_;

    /* ===== Pipeline Flags ===== */
    VkPipelineCreateFlags flags_;

    /* ===== Pipeline States ===== */
    hk::vector<VkPipelineShaderStageCreateInfo> shader_stages_;
    VkPipelineVertexInputStateCreateInfo   vertex_input_;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_;
    VkPipelineTessellationStateCreateInfo  tessellation_;
    VkPipelineViewportStateCreateInfo      viewport_;
    VkPipelineRasterizationStateCreateInfo rasterizer_;
    VkPipelineMultisampleStateCreateInfo   multisampling_;
    VkPipelineDepthStencilStateCreateInfo  depth_stencil_;
    VkPipelineColorBlendStateCreateInfo    color_blending_;
    VkPipelineDynamicStateCreateInfo       dynamic_state_;

    // VkPipeline basePipelineHandle;
    // u32 basePipelineIndex;

    // Data needed to be kept alive till pipeline creation or just needed later
    /* ===== Pipeline Layout Data ===== */
    hk::vector<VkPushConstantRange> push_ranges_;
    hk::vector<VkDescriptorSetLayout> desc_layouts_;

    /* ===== Vertex State Data ===== */
    VkVertexInputBindingDescription vertex_binding_;
    hk::vector<VkVertexInputAttributeDescription> attribute_descs_;

    /* ===== Viewport State Data ===== */
    hk::vector<VkViewport> viewports_;
    hk::vector<VkRect2D> scissors_;

    /* ===== Color Blending State Data ===== */
    hk::vector<VkPipelineColorBlendAttachmentState> blend_states_;

    /* ===== Dynamic State Data ===== */
    hk::vector<VkDynamicState> dynamic_states_;

    /* ===== Pipeline Extending Stages ===== */
    hk::vector<VkFormat> colors_;
    VkPipelineRenderingCreateInfo rendering_info_;

    /* ===== Hikai Pipeline Wrapper ===== */
    hk::Pipeline out_;

    // Convenience
    VkDevice device;
    VkPhysicalDeviceLimits limits;
    VkPhysicalDeviceFeatures features;
};

}

#endif // HK_PIPELINE_H
