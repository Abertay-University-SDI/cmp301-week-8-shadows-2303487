/**
 * shadow_vs.hlsl
 * --------------
 * Vertex shader for shadow mapping with both directional and spot lights.
 * Transforms vertices into the necessary spaces for shadow mapping and lighting calculations.
 * Outputs world position, projected positions in light spaces, and interpolates texture and normals.
 */

 // Vertex shader for shadow mapping with two directional lights and a spotlight.

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix dirLightViewMatrix;
    matrix dirLightProjMatrix;
    matrix spotLightViewMatrix;
    matrix spotLightProjMatrix;
    matrix secondDirLightViewMatrix;
    matrix secondDirLightProjMatrix;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 dirLightViewPos : TEXCOORD1;
    float4 spotLightViewPos : TEXCOORD2;
    float4 worldPos : TEXCOORD3;
    float4 secondDirLightViewPos : TEXCOORD4;
};

OutputType main(InputType input)
{
    OutputType output;

    float4 worldPos = mul(input.position, worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projectionMatrix);

    float4 dirView = mul(worldPos, dirLightViewMatrix);
    output.dirLightViewPos = mul(dirView, dirLightProjMatrix);

    float4 spotView = mul(worldPos, spotLightViewMatrix);
    output.spotLightViewPos = mul(spotView, spotLightProjMatrix);

    float4 secondDirView = mul(worldPos, secondDirLightViewMatrix);
    output.secondDirLightViewPos = mul(secondDirView, secondDirLightProjMatrix);

    output.tex = input.tex;
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.worldPos = worldPos;
    return output;
}