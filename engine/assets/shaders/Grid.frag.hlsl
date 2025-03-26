#include "globals.hlsli"
#include "utils.hlsli"

struct PixelInput {
    float4 pos : SV_Position;
    float2 tc : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target0
{
    float4 o = 0.f;

    float grid_size = 2.0f;

    float cell_size = 1.0f;
    float half_cell_size = cell_size * 0.5f;

    float subcell_size = 0.1f;
    float half_subcell_size = subcell_size * 0.5f;

    float cell_line_thickness    = 0.01f;
    float subcell_line_thickness = 0.001f;

    float4 cell_colour    = float4(0.75, 0.75, 0.75, 0.5);
    float4 subcell_colour = float4( 0.5,  0.5,  0.5, 0.5);

    float min_fade_distance = grid_size * 0.05f;
    float max_fade_distance = grid_size * 0.5f;
    float height_to_fade_distance_ratio = 25.0f;

    float2 cell_coords    = mod(input.tc + half_cell_size,    cell_size);
    float2 subcell_coords = mod(input.tc + half_subcell_size, subcell_size);

    float d = fwidth(input.tc).x;
    float adjusted_cell_line_thickness    = 0.5 * (cell_line_thickness    + d);
    float adjusted_subcell_line_thickness = 0.5 * (subcell_line_thickness + d);;

    float2 distance_to_cell    = abs(cell_coords    - half_cell_size);
    float2 distance_to_subcell = abs(subcell_coords - half_subcell_size);

    if (any(distance_to_cell    < adjusted_cell_line_thickness    * 0.5)) o = cell_colour;
    if (any(distance_to_subcell < adjusted_subcell_line_thickness * 0.5)) o = subcell_colour;

    float fade_distance = abs(camera.pos.y) * height_to_fade_distance_ratio;
    fade_distance = max(fade_distance, min_fade_distance);
    fade_distance = min(fade_distance, max_fade_distance);

    // FIX: temp, until I fix the camera
    fade_distance = 25.f;

    float distance_to_camera = length(input.tc - camera.pos.xz);
    float opacity_falloff = smoothstep(1.0, 0.0, distance_to_camera / fade_distance);

    return o * opacity_falloff;

    //////////////////////////////////////////////
}
