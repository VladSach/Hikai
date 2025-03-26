#ifndef HK_GLOBALS_HLSLI
#define HK_GLOBALS_HLSLI

// Defines
static const float PI = 3.14159265358979323846f;

// Per-Frame data
cbuffer GlobalData : register(b0, space0) {
    struct CameraData {
        float3 pos; // In world space
        float4x4 view_proj;
    } camera;

    struct FrameData {
        float2 resolution;
        float time;
    } frame;
};

cbuffer LightsData : register(b1, space0) {
    struct PointLight {
        float4 color;
        float intensity;
        float3 position;
    };

    struct SpotLight {
        float4 color;
        float3 direction;

        float inner_cutoff;
        float outer_cutoff;

        float3 position;
    };

    struct DirectionalLight {
        float3 color;
        float3 direction;

        float pad;
    };

    struct Lights {
        SpotLight spots[3];
        PointLight points[3];
        DirectionalLight directionals[3];

        uint spot_count;
        uint point_count;
        uint directional_count;
    } lights;
}

/* ===== Samplers ===== */
namespace hk {
namespace sampler {
SamplerState nearest_repeat : register(s2);
SamplerState nearest_mirror : register(s3);
SamplerState nearest_clamp  : register(s4);
SamplerState nearest_border : register(s5);

SamplerState linear_repeat : register(s6);
SamplerState linear_mirror : register(s7);
SamplerState linear_clamp  : register(s8);
SamplerState linear_border : register(s9);

SamplerState anisotropic_repeat : register(s10);
SamplerState anisotropic_mirror : register(s11);
SamplerState anisotropic_clamp  : register(s12);
SamplerState anisotropic_border : register(s13);
}
}

#endif // HK_GLOBALS_HLSLI
