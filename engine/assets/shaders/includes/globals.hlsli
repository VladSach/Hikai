#ifndef HK_GLOBALS_HLSLI
#define HK_GLOBALS_HLSLI

cbuffer PerFrame : register(b0) {
    float2 resolution;
    float time;
    float3 cameraPos;
    float4x4 viewProj;
};


// GLSL-like mod function
#define mod(x, y) ((x) - (y) * floor((x)/(y)))



#endif // HK_GLOBALS_HLSLI
