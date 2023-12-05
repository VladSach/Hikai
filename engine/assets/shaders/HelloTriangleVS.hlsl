//struct VertexInput {
//    float3 position : POS;
//};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

//VertexOutput main(VertexInput input) {
VertexOutput main(uint index: SV_VertexID) {
    VertexOutput output;
    float3 position[3] = {
        {0.f, -0.5f, 0.f},
        {0.5f, 0.5f, 0.f},
        {-0.5f, 0.5, 0.f},
    };
    float3 color[3] = {
        {1.f, 0.f, 0.f},
        {0.f, 1.f, 0.f},
        {0.f, 0.f, 1.f}
    };

    output.position = float4(position[index], 1.f);
    output.color = float3(color[index]);
    return output;
}
