[[vk::push_constant]]
struct ShapeDesc {
    float3 color;

    float psize;
    bool use_depth;
} desc;

[[vk::input_attachment_index(4)]]
SubpassInput<float> depth: register(t4, space1);

float4 main(float4 pos : SV_Position) : SV_Target0
{
    if (desc.use_depth) {
        if (pos.z <= depth.SubpassLoad()) {
            discard;
        }
    }
    return float4(desc.color.xyz, 1.f);
}
