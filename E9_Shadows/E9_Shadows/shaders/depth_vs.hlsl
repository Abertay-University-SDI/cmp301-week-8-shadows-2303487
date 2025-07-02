/**
 * depth_vs.hlsl
 * -------------
 * Vertex shader for depth rendering.
 * Transforms vertex positions through world, view, and projection matrices,
 * passing projected position for use in depth calculation in the pixel shader.
 */

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;        // Projected position for rasterization.
    float4 depthPosition : TEXCOORD0;     // Projected position for depth interpolation.
};

OutputType main(InputType input)
{
    OutputType output;

    // Transform the vertex to world, view, and projection space
    float4 worldPos = mul(input.position, worldMatrix);
    float4 viewPos = mul(worldPos, viewMatrix);
    output.position = mul(viewPos, projectionMatrix);
    output.depthPosition = output.position; // Pass projected position for depth calculation

    return output;
}