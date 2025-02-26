#ifndef HK_GLOBALS_HLSLI
#define HK_GLOBALS_HLSLI

cbuffer PerFrame : register(b0) {
    float2 resolution;
    float3 cameraPos;
    float time;
    float4x4 viewProj;
};

cbuffer Lights : register(b1) {
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

    PointLight pointlights[3];
    SpotLight spotlights[3];
    DirectionalLight directional;

    uint spotLightsNum;
    uint pointLightsNum;
}

// TODO: move out of global
[[vk::push_constant]]
struct ModelToWorld {
    float4x4 mat;
} modelToWorld;

// GLSL-like mod function
#define mod(x, y) ((x) - (y) * floor((x)/(y)))

#endif // HK_GLOBALS_HLSLI
