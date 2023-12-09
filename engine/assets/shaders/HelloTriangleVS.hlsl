struct VertexInput {
    //[[vk::location(0)]]
    float3 position : POSITION0;

    //[[vk::location(1)]]
    float3 color : COLOR0;
};

struct VertexOutput {
    float4 position : SV_Position;

    //[[vk::location(0)]]
    float3 color : COLOR;
};

VertexOutput main(VertexInput input, uint index: SV_VertexID) {
    VertexOutput output;

    output.color = input.color;
    output.position = float4(input.position, 1.f);

    return output;
}
