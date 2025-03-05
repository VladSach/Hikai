#include "globals.hlsli"
#include "pbr.hlsli"

struct PixelInput {
    float4 sv_pos : SV_Position;
    float2 tc : TEXCOORD0;
};

// https://github.com/Microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst#subpass-inputs
[[vk::input_attachment_index(0)]]
SubpassInput<float4> posAtt: register(t0, space1);
[[vk::input_attachment_index(1)]]
SubpassInput<float4> normalAtt: register(t1, space1);
[[vk::input_attachment_index(2)]]
SubpassInput<float4> albedoAtt: register(t2, space1);
[[vk::input_attachment_index(3)]]
SubpassInput<float4> materialAtt: register(t3, space1);
[[vk::input_attachment_index(4)]]
SubpassInput<float> depthAtt: register(t4, space1);

float4 main(PixelInput input) : SV_Target0
{
    float3 world_pos = posAtt.SubpassLoad().xyz;
    float4 albedo = pow(albedoAtt.SubpassLoad(), 2.2f);
    float3 normal = normalAtt.SubpassLoad().xyz;
    float4 material = materialAtt.SubpassLoad();
    float depth = depthAtt.SubpassLoad();
    float metallic = material.x;
    float roughness = material.y;

    float3 V = normalize(cameraPos - world_pos);
    float NdotV = max(dot(normal, V), 0.f);

    float3 F0 = lerp((.04f).xxx, albedo.rgb, metallic);

    PointLight light = pointlights[0];

    float3 L = normalize(light.position - world_pos);
    float3 H = normalize(L + V);
    float NdotH = max(dot(normal, H), 0.f);
    float NdotL = max(dot(normal, L), 0.f);
    float VdotH = max(dot(V, H), 0.f);

    float3 F = fresnel(VdotH, F0);
    float  D = ggx(pow(roughness, 4), NdotH);
    float  G = smith(pow(roughness, 4), NdotV, NdotL);

    // Cook-Torrance
    float3 nominator = D * G * F;
    float denom = 4.f * NdotV * NdotL;
    float3 specular = nominator / max(denom, .0001f);

    float distance = length(light.position - world_pos);
    float attenuation = 1.f / (distance * distance);
    float3 radiance = light.color.rgb * attenuation;

    float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= 1.f - metallic;

    float3 diffuse = (albedo.xyz * kD / PI + specular) * radiance * NdotL;

    return float4(diffuse + albedo.xyz, albedo.w);
    //return float4(normal, 1);
    //return float4(world_pos, 1);
}
