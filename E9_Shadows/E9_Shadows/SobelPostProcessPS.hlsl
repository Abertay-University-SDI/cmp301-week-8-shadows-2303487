// SobelPostProcessPS.hlsl
Texture2D sceneTex : register(t0);
SamplerState samplerState : register(s0);

cbuffer ScreenSizeBuffer : register(b0)
{
    float2 texelSize;
    float2 padding;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float luminance(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

float2 ClampUV(float2 uv) { return saturate(uv); }

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 tex = input.tex;
    float tl = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(-1, -1))).rgb);
    float tc = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(0, -1))).rgb);
    float tr = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(1, -1))).rgb);
    float ml = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(-1,  0))).rgb);
    float mr = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(1,  0))).rgb);
    float bl = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(-1,  1))).rgb);
    float bc = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(0,  1))).rgb);
    float br = luminance(sceneTex.Sample(samplerState, ClampUV(tex + texelSize * float2(1,  1))).rgb);

    float gx = -tl - 2 * ml - bl + tr + 2 * mr + br;
    float gy = -tl - 2 * tc - tr + bl + 2 * bc + br;
    float edge = sqrt(gx * gx + gy * gy);
    edge = edge * 0.5; // Try 0.1, 0.05, 0.01 -- lower if still too bright
    edge = saturate(edge);
    return float4(edge, edge, edge, 1.0f);
}