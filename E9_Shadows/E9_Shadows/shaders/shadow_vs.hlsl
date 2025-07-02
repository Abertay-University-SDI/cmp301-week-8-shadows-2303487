/**
 * shadow_vs.hlsl
 * --------------
 * Vertex shader for shadow mapping with both directional and spot lights.
 * Transforms vertices into the necessary spaces for shadow mapping and lighting calculations.
 * Outputs world position, projected positions in light spaces, and interpolates texture and normals.
 */

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix;
    matrix lightProjectionMatrix;
    matrix spotLightViewMatrix;
    matrix spotLightProjectionMatrix;
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
    float4 lightViewPos : TEXCOORD1;
    float4 spotLightViewPos : TEXCOORD2;
    float4 worldPos : TEXCOORD3;
};

OutputType main(InputType input)
{
    OutputType output;

    // Transform vertex to world, view, and projection space
    float4 worldPos = mul(input.position, worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projectionMatrix);

    // Directional light view-projection
    float4 lightView = mul(worldPos, lightViewMatrix);
    output.lightViewPos = mul(lightView, lightProjectionMatrix);

    // Spot light view-projection
    float4 spotView = mul(worldPos, spotLightViewMatrix);
    output.spotLightViewPos = mul(spotView, spotLightProjectionMatrix);

    output.tex = input.tex;
    output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));
    output.worldPos = worldPos;
    return output;
}