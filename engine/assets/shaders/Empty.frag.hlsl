struct PixelOutput
{
    float4 position: SV_Target0;
    float4 normal  : SV_Target1;
    float4 albedo  : SV_Target2;
    float4 material: SV_Target3;
    float4 color   : SV_Target4;
};

PixelOutput main()
{
    PixelOutput output;
    output.position = 0.f;
    output.normal = 0.f;
    output.albedo = 0.f;
    output.material = 0.f;
    output.color = 0.f;
    return output;
}
