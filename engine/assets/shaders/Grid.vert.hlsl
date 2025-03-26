#include "globals.hlsli"

struct VertexOutput {
    float4 position : SV_Position;
    float2 tc : TEXCOORD0;
};

static const float grid_size = 100.0f;
static const float4 positions[4] = {
    { -0.5, 0.0,  0.5, 1.0 },
    {  0.5, 0.0,  0.5, 1.0 },
    { -0.5, 0.0, -0.5, 1.0 },
    {  0.5, 0.0, -0.5, 1.0 }
};

VertexOutput main(uint id : SV_VertexID) {
    VertexOutput output;

    float4 pos = positions[id];
    pos.xyz *= grid_size;
    //pos.xz += cameraPos.xz;

    float4 world_pos = mul(camera.view_proj, pos);

    float div = max(2.0, round(grid_size));
    float3 camera_offset = floor(camera.pos / div) * div;

    output.tc = pos.xz;
    output.position = world_pos;
    //output.tc.yx = (world_pos.xyz - camera_offset).xy;
    //output.tc.wz = world_pos.xy;
    return output;
}
