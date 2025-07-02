// SobelPostProcessVS.hlsl

struct VS_INPUT
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.position = float4(input.position, 1.0f);
    output.tex = input.tex;
    return output;
}