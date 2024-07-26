cbuffer PerFrame : register(b0) {
    float2 resolution;
    float time;
    float4x4 viewProj;
};

struct VertexInput {
    float3 pos : POSITION0;
    float3 normal : COLOR0;
    float2 tc : TEXCOORD0;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 tc : TEXCOORD0;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;

    output.position = mul(viewProj, float4(input.pos, 1.f));
    //output.position = float4(input.pos, 1.f);
    output.normal = input.normal;
    output.tc = input.tc;

    return output;
}
