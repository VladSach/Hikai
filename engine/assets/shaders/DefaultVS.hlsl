#include "globals.hlsli"

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

    float scale = 0.1f; // Scaling factor
    float rotationAngle = 0.9f * time; // Rotation angle in radians

    // Create scaling matrix
    float3x3 scaleMatrix = float3x3(
        scale, 0, 0,
        0, scale, 0,
        0, 0, scale
    );

    // Create rotation matrix around Y-axis
    float3x3 rotationMatrix = float3x3(
        cos(rotationAngle), 0, sin(rotationAngle),
        0, 1, 0,
        -sin(rotationAngle), 0, cos(rotationAngle)
    );

    // Calculate translation vector
    float3 translationVector = float3(0, 10, 0);

    float3 transformedPos = input.pos + translationVector;
    transformedPos = mul(transformedPos, scaleMatrix);
    transformedPos = mul(float3(transformedPos), rotationMatrix);

    output.position = mul(viewProj, float4(transformedPos, 1.f));
    output.normal = input.normal;
    output.tc = input.tc;

    return output;
}
