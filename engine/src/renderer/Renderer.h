#ifndef HK_RENDER_DEVICE_H
#define HK_RENDER_DEVICE_H

#include "platform/platform.h"
#include "vendor/vulkan/vulkan.h"

#include "renderer/VulkanContext.h"
#include "renderer/DrawContext.h"

#include "renderer/vkwrappers/Image.h"
#include "renderer/vkwrappers/Pipeline.h"
#include "renderer/vkwrappers/Swapchain.h"
#include "renderer/vkwrappers/Descriptors.h"

#include "renderer/renderpass/UIPass.h"
#include "renderer/renderpass/PresentPass.h"
#include "renderer/renderpass/OffscreenPass.h"

#include "math/hkmath.h"

#include "core/events.h"
#include "utils/containers/hkvector.h"

// FIX: temp
struct ModelToWorld {
    hkm::mat4f transform;
};

// FIX: temp
// https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer
struct SceneData {
    hkm::vec2f resolution;
    u64 pad;
    hkm::vec3f cameraPosition;
    f32 time;
    hkm::mat4f viewProjection;
};

struct LightSources {
    // SpotLight spotlights[maxLightsSize];
    hk::PointLight pointlights[3];
    // DirectionalLight directional;

    u32 spotlightsSize = 0;
    u32 pointlightsSize = 0;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { deinit(); }

    void init(const Window *window);
    void deinit();

    void draw(hk::DrawContext &context);

    static void resize(const hk::event::EventContext &size, void *listener);

    // FIX: temp
    inline void updateFrameData(const SceneData &ubo)
    {
        frame_data = ubo;
        frame_data_buffer.update(&frame_data);
    }
    inline void updateLights(const LightSources &ubo)
    {
        lights = ubo;
        lights_buffer.update(&lights);
    }

// FIX: temp public
public:
    // FIX: temp
    b8 image_changed_ = false;

    // FIX: temp, maybe move to offscreen_
    SceneData frame_data;
    hk::Buffer frame_data_buffer;
    LightSources lights;
    hk::Buffer lights_buffer;

    // TODO: probably don't need it
    const Window *window_;

    hk::Swapchain swapchain_;

    b8 resized = false;

    b8 use_ui_ = true;

    // TODO: move to offscreen
    VkDescriptorSetLayout sceneDescriptorLayout;

    struct FrameData {
        VkCommandBuffer cmd = VK_NULL_HANDLE;

        VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
        VkSemaphore submit_semaphore  = VK_NULL_HANDLE;
        VkFence in_flight_fence       = VK_NULL_HANDLE;

        hk::DescriptorAllocator descriptor_alloc;
    };

    hk::vector<FrameData> frames_;
    u32 current_frame_ = 0;
    u32 max_frames_ = 2;

    hk::UIPass ui_;
    hk::PresentPass present_;
    hk::OffscreenPass offscreen_;

    VkSampler samplerLinear;
    VkSampler samplerNearest;

    u32 hndlDefaultVS;
    u32 hndlDefaultPS;
    u32 hndlNormalsPS;
    u32 hndlTexturePS;
    u32 hndlPhong;

    hk::Pipeline gridPipeline;
    u32 hndlGridVS;
    u32 hndlGridPS;

    u32 curShaderVS;
    u32 curShaderPS;

    // Convenience
    VkDevice device_;
    VkPhysicalDevice physical_;

private:
    void createFrameResources();

    // FIX: remove
    void createGridPipeline();

    void loadShaders();
    void createSamplers();
};

#endif // HK_RENDER_DEVICE_H
