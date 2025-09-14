#ifndef HK_RENDER_DEVICE_H
#define HK_RENDER_DEVICE_H

#include "platform/platform.h"

#include "hkvulkan.h"

#include "renderer/DrawContext.h"

#include "renderer/renderpass/UIPass.h"
#include "renderer/renderpass/PresentPass.h"
#include "renderer/renderpass/OffscreenPass.h"
#include "renderer/renderpass/PostProcessPass.h"

#include "core/events.h"

#include "hkstl/math/hkmath.h"
#include "hkstl/containers/hkvector.h"

// FIX: temp
struct InstanceData {
    hkm::mat4f model_to_world; // transform
};

// FIX: temp
// https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer
struct SceneData {
    // Camera
    hkm::vec4f pos; // used only .xyz
    hkm::mat4f view_proj;

    // Frame
    hkm::vec2f resolution;
    f32 time;
};

struct LightSources {
    struct PointLight {
        hkm::vec4f color;
        f32 intensity;
        hkm::vec3f pos;
    };

    struct SpotLight {
        hkm::vec4f color;
        hkm::vec3f dir;

        f32 inner_cutoff;
        f32 outer_cutoff;

        hkm::vec3f pos;
    };

    struct DirectionalLight {
        hkm::vec4f color;
        hkm::vec3f dir;

        f32 pad;
    };

    SpotLight spot_lights[3];
    PointLight point_lights[3];
    DirectionalLight directional_lights[3];

    u32 spot_count = 0;
    u32 point_count = 0;
    u32 directinal_count = 0;
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
        hk::bkr::update_buffer(frame_data_buffer, &frame_data);
    }
    inline void updateLights(const LightSources &ubo)
    {
        lights = ubo;
        hk::bkr::update_buffer(lights_buffer, &lights);
    }

// FIX: temp public
public:
    // TODO: probably don't need it
    const Window *window_;

    hk::Swapchain swapchain_;

    // Passes
    // TODO:
    // Shadow pass
    // gbuffer pass
    // light pass
    // ? separate blur pass
    // post process
    // ui pass
    // present pass

    b8 resized = false;

    b8 use_ui_ = true;

    struct BindlessDescriptor {
        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE;
    } bindless_;

    // TODO: move to offscreen
    // Scene Data
    SceneData frame_data;
    hk::BufferHandle frame_data_buffer;
    LightSources lights;
    hk::BufferHandle lights_buffer;

    struct FrameData {
        VkCommandBuffer cmd = VK_NULL_HANDLE;

        VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
        VkSemaphore submit_semaphore  = VK_NULL_HANDLE;
        VkFence in_flight_fence       = VK_NULL_HANDLE;
    };

    hk::vector<FrameData> frames_;
    u32 current_frame_ = 0;
    u32 max_frames_ = 2;

    hk::UIPass ui_;
    hk::PresentPass present_;
    hk::OffscreenPass offscreen_;
    hk::PostProcessPass post_process_;

    // Global Samplers
    struct Samplers {
        struct Linear {
            VkSampler repeat;
            VkSampler mirror;
            VkSampler clamp;
            VkSampler border;
        } linear;
        struct Nearest {
            VkSampler repeat;
            VkSampler mirror;
            VkSampler clamp;
            VkSampler border;
        } nearest;
        struct Anisotopic {
            VkSampler repeat;
            VkSampler mirror;
            VkSampler clamp;
            VkSampler border;
        } anisotropic;
    } samplers_;
    // VkSampler samplers_[12];

    u32 hndlDefaultVS;
    u32 hndlDefaultPS;
    u32 hndlNormalsPS;
    u32 hndlPBR;

    hk::Pipeline gridPipeline;
    u32 hndlGridVS;
    u32 hndlGridPS;

    // Convenience
    VkDevice device_;
    VkPhysicalDevice physical_;

private:
    void createFrameResources();

    void createBindlessDescriptor();

    // FIX: remove
    void createGridPipeline();

    void loadShaders();
    void createSamplers();
};

#endif // HK_RENDER_DEVICE_H
