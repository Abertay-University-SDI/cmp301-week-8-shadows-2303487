/**
 * texture_ps.hlsl
 * ---------------
 * Pixel (fragment) shader for basic texture mapping.
 * Samples a texture using interpolated UV coordinates and outputs the resulting color.
 */

Texture2D texture0 : register(t0);      // Texture input.
SamplerState Sampler0 : register(s0);   // Sampler state for texture sampling.

struct InputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

// Main pixel shader: Samples the texture at the given UV and returns the color.
float4 main(InputType input) : SV_TARGET
{
	return texture0.Sample(Sampler0, input.tex);
}