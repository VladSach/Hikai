#define HELLO_TRIANGLE 1
#define CURLESQUE 0
#define PLASMA_PARTICLES 0

//cbuffer PerFrame : register(b0) {
//    float4 resolution;
//    float time;
//};

struct PixelInput {
    float4 position : SV_Position;
    float3 color : COLOR;
};

struct PixelOutput {
    float4 attachment0 : SV_Target0;
};

#if HELLO_TRIANGLE
PixelOutput main(PixelInput pixelInput)
{
    float3 inColor = pixelInput.color;
    PixelOutput output;
    output.attachment0 = float4(inColor, 1.0f);
    return output;
}
#endif

#if CURLESQUE
#define pi 3.14159265359
float saw(float x)
{
    return abs(frac(x) - 0.5)*2.0;
}

float dw(float2 p, float2 c, float t)
{
    return sin(length(p - c) - t);
}

float dw1(float2 uv)
{
    float v = 0.0;
    float t = time * 2.0;
    v += dw(uv, float2(sin(t*0.07)*30.0, cos(t*0.04)*20.0), t*1.3);
    v += dw(uv, float2(cos(t*0.13)*30.0, sin(t*0.14)*20.0), t*1.6 + 1.0);
    v += dw(uv, float2(18, -15), t*0.7 + 2.0);
    v += dw(uv, float2(-18, 15), t*1.1 - 1.0);
    return v / 4.0;
}

float fun(float x, float y)
{
    return dw1(float2(x - 0.5, y - 0.5)*80.0);
}

float3 duv(float2 uv)
{
    float x = uv.x;
    float y = uv.y;
    float v = fun(x, y);
    float d = 1.0 / 400.0;
    float dx = (v - fun(x + d, y)) / d;
    float dy = (v - fun(x, y + d)) / d;
    float a = atan2(dx, dy) / pi / 2.0;
    return float3(v, 0, (v*4.0 + a));
}

PixelOutput main(PixelInput pixelInput)
{
    PixelOutput output;
    float2 uv = pixelInput.position.xy / resolution.x;
    float3 h = duv(uv);
    float sp = saw(h.z + time * 1.3);
    //sp=(sp>0.5)?0.3:1.0;
    sp = clamp((sp - 0.25)*2.0, 0.5, 1.0);
    output.attachment0 = float4((h.x + 0.5)*sp, (0.3 + saw(h.x + 0.5)*0.6)*sp, (0.6 - h.x)*sp, 1.0);
    return output;
}
#endif

#if PLASMA_PARTICLES
float noise(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

PixelOutput main(PixelInput pixelInput)
{
    float2 uv = pixelInput.position.xy / resolution.xy;

    float u_brightness = 1.2;
    float u_blobiness = 0.9;
    float u_particles = 140.0;
    float u_limit = 70.0;
    float u_energy = 1.0 * 0.75;

    float2 position = (pixelInput.position.xy / resolution.x);
    float t = time * u_energy;

    float a = 0.0;
    float b = 0.0;
    float c = 0.0;

    float2 pos;

    float2 center = float2(0.5, 0.5 * (resolution.y / resolution.x));

    float na, nb, nc, nd, d;
    float limit = u_particles / u_limit;
    float step = 1.0 / u_particles;
    float n = 0.0;

    for (float i = 0.0; i <= 1.0; i += 0.025) {

        if (i <= limit) {

            float2 np = float2(n, 1 - 1);

            na = noise(np * 1.1);
            nb = noise(np * 2.8);
            nc = noise(np * 0.7);
            nd = noise(np * 3.2);

            pos = center;
            pos.x += sin(t*na) * cos(t*nb) * tan(t*na*0.15) * 0.3;
            pos.y += tan(t*nc) * sin(t*nd) * 0.1;

            d = pow(1.6*na / length(pos - position), u_blobiness);

            if (i < limit * 0.3333) a += d;
            else if (i < limit * 0.5) b += d;
            else c += d;

            n += step;
        }
    }

    float3 col = float3(a*25.5, 0.0, a*b) * 0.0001 * u_brightness;

    PixelOutput output;
    output.attachment0 = float4(col, 1.0);

    //fragColor = vec4(uv,0.5+0.5*sin(iTime),1.0);
    return output;
}
#endif
