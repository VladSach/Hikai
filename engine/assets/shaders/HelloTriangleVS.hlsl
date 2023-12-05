struct VertexInput {
    float3 position : POS;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

VertexInput main(VertexOutput input) {
    VertexOutput output;
    output.position = float4(input.position, 1.f);
    output.color = float4(.0f);
    return output;
}
