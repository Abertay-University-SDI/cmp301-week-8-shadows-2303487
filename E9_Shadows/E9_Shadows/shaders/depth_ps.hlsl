/**
 * depth_ps.hlsl
 * -------------
 * Pixel (fragment) shader for depth rendering.
 * Converts projected position to linear depth and outputs it as a grayscale color.
 * Used for generating depth maps for effects like shadow mapping.
 */

struct InputType
{
    float4 position : SV_POSITION;        // Screen-space position (after projection).
    float4 depthPosition : TEXCOORD0;     // Position in projection space for depth calculation.
};

// Main pixel shader: Outputs normalized depth as grayscale.
float4 main(InputType input) : SV_TARGET
{
    // Perspective divide to retrieve normalized depth (z/w)
    float depthValue = input.depthPosition.z / input.depthPosition.w;
    return float4(depthValue, depthValue, depthValue, 1.0f);
}