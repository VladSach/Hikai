#include "globals.hlsli"

cbuffer MaterialConstants : register(b0, space2) {
    float4 color;
    float4 specular;
    float4 ambient;

    float opacity;

    float metalness;
    float roughness;
};

Texture2D<float4> diffuseMap   : register(t1, space2);
Texture2D<float4> normalMap    : register(t2, space2);
Texture2D<float4> emissiveMap  : register(t3, space2);
Texture2D<float4> metalnessMap : register(t4, space2);
Texture2D<float4> roughnessMap : register(t5, space2);
Texture2D<float4> lightMap     : register(t6, space2);

struct PixelInput {
    float4 sv_pos : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
    float3 pos : POSITION0;
    float3 tangent : TANGENT;
};

struct PixelOutput
{
    float4 position : SV_Target0;
    float4 normal   : SV_Target1;
    float4 albedo   : SV_Target2;
    float4 material : SV_Target3;
    float4 color    : SV_Target4;
};

PixelOutput main(PixelInput input)
{
    PixelOutput output;

    float4 albedo;
    float3 normal;
    float metallic;
    float rough;
    float ao;

    float4 diffuse_value = diffuseMap.Sample(hk::sampler::linear_repeat, input.tc);
    float3 normal_value = normalMap.Sample(hk::sampler::linear_repeat, input.tc).rgb;
    float metallic_value = metalnessMap.Sample(hk::sampler::linear_repeat, input.tc).r;
    float roughness_value = roughnessMap.Sample(hk::sampler::linear_repeat, input.tc).r;
    float ao_value = lightMap.Sample(hk::sampler::linear_repeat, input.tc).r;

    // https://iquilezles.org/articles/gpuconditionals/
    albedo = all(diffuse_value == (.0f).xxxx) ? float4(color.rgb, 1.f) : diffuse_value;
    normal = all(normal_value  == (.0f).xxx)  ? input.normal           : normal_value;

    metallic = metallic_value  == .0f ? metalness : metalness * metallic_value;
    rough    = roughness_value == .0f ? roughness : roughness * roughness_value;

    ao = ao_value == .0f ? 1 : ao_value;

    if (any(normal != input.normal)) {
        float3 B = cross(input.normal, input.tangent);
        float3x3 TBN = float3x3(input.tangent, B, input.normal);
        normal = normalize(mul(TBN, normalize(normal * 2.f - 1.f)));
    }

    output.position = float4(input.pos, 1.f);
    output.normal = float4(normal, 1.f);
    output.albedo = albedo;
    output.material.r = metallic;
    output.material.g = rough;
    output.material.b = ao;
    output.material.a = 1.f;

    // Write to avoid undefined behaviour (validation error)
    output.color = 0.f;
    return output;
}
