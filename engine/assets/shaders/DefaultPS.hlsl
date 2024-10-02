#include "globals.hlsli"

struct PixelInput {
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target0
{
    return float4(1.f, 1.f, 1.f, 1.f);
}
