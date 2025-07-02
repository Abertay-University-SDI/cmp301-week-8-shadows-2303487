/**
 * texture_vs.hlsl
 * ---------------
 * Vertex shader for basic texture mapping.
 * Transforms vertex positions and passes through UVs and normals for future use (e.g., lighting).
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
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

// Main vertex shader: Transforms vertex position and passes UV/normal to pixel shader.
OutputType main(InputType input)
{
	OutputType output;
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);
	output.tex = input.tex;
	output.normal = input.normal; // Forward normal for future effects
	return output;
}