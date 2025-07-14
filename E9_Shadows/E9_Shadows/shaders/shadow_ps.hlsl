/**
 * shadow_ps.hlsl
 * --------------
 * Pixel shader for shadow mapping with both directional and spot lights in a scene.
 * Calculates shadow contributions from both lights using their respective shadow maps and combines them with texture and lighting.
 * Handles light attenuation, shadow biasing, and combines multiple light sources for realistic shading.
 * supports debug visualization of shadows.
 * allows for toggling between normal rendering and shadow visualization.
 * works with a texture sampler and shadow samplers for both directional and spot lights.
  * Outputs the final color of the pixel, either as a lit color or a shadow visualization based on the debug mode.
 */
 // Pixel shader for shadow mapping with two directional lights and a spotlight.
 // Calculates shadow contributions from all three lights using their respective shadow maps.

Texture2D shaderTexture : register(t0);
Texture2D dirShadowMapTexture : register(t1);
Texture2D spotShadowMapTexture : register(t2);
Texture2D secondDirShadowMapTexture : register(t3);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b1)
{
    // Directional light 1
    float4 dirAmbient;
    float4 dirDiffuse;
    float3 dirDirection;
    float  pad0;

    // Spotlight
    float4 spotAmbient;
    float4 spotDiffuse;
    float3 spotDirection;
    float  spotCutoff;
    float3 spotPosition;
    float  spotExponent;

    // Directional light 2
    float4 secondDirAmbient;
    float4 secondDirDiffuse;
    float3 secondDirDirection;
    float pad1;

    int    shadowDebug;
    float3 pad2; // alignment
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

float2 getProjectiveCoords(float4 lightViewPosition)
{
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

bool hasDepthData(float2 uv)
{
    return (uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f);
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    float depthValue = sMap.Sample(shadowSampler, uv).r;
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    lightDepthValue -= bias;
    return (lightDepthValue >= depthValue);
}

float4 calculateLighting(float3 lightDirection, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(diffuse * intensity);
    return colour;
}

float4 main(OutputType input) : SV_TARGET
{
    float shadowMapBias = 0.0005f;
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

    // --- Directional Light 1 Shadow ---
    float2 dirTexCoord = getProjectiveCoords(input.dirLightViewPos);
    float dirShadow = 1.0f;
    if (hasDepthData(dirTexCoord))
        dirShadow = !isInShadow(dirShadowMapTexture, dirTexCoord, input.dirLightViewPos, shadowMapBias);

    // --- Spotlight Shadow ---
    float2 spotTexCoord = getProjectiveCoords(input.spotLightViewPos);
    float spotShadow = 1.0f;
    if (hasDepthData(spotTexCoord))
        spotShadow = !isInShadow(spotShadowMapTexture, spotTexCoord, input.spotLightViewPos, shadowMapBias);

    // --- Directional Light 2 Shadow ---
    float2 secondDirTexCoord = getProjectiveCoords(input.secondDirLightViewPos);
    float secondDirShadow = 1.0f;
    if (hasDepthData(secondDirTexCoord))
        secondDirShadow = !isInShadow(secondDirShadowMapTexture, secondDirTexCoord, input.secondDirLightViewPos, shadowMapBias);

    // Shadow debug coloring
    float dirShadowArea = (dirShadow < 1.0) ? 1.0 : 0.0;
    float spotShadowArea = (spotShadow < 1.0) ? 1.0 : 0.0;
    float secondDirShadowArea = (secondDirShadow < 1.0) ? 1.0 : 0.0;

    float3 normal = input.normal;

    // --- Directional Light 1 Calculation ---
    float4 dirLightCol = calculateLighting(-dirDirection, normal, dirDiffuse) * dirShadow;

    // --- Directional Light 2 Calculation ---
    float4 secondDirLightCol = calculateLighting(-secondDirDirection, normal, secondDirDiffuse) * secondDirShadow;

    // --- Spotlight Calculation ---
    float3 lightToPixel = normalize(input.worldPos.xyz - spotPosition);
    float spotFactor = dot(normalize(spotDirection), lightToPixel);
    float spotLightVal = 0;
    if (spotFactor > spotCutoff)
        spotLightVal = pow(spotFactor, spotExponent);
    float4 spotLightCol = calculateLighting(normalize(spotDirection), normal, spotDiffuse) * spotLightVal * spotShadow;

    float4 colour =
        dirAmbient + dirLightCol +
        spotAmbient + spotLightCol +
        secondDirAmbient + secondDirLightCol;

    colour = saturate(colour) * textureColour;

    // --- Debug visualization: show shadowed regions for each light ---
    if (shadowDebug == 1)
    {
        if (dirShadowArea > 0 && spotShadowArea > 0 && secondDirShadowArea > 0)
            return float4(0, 0, 0, 1); // Black, shadowed by all
        else if (dirShadowArea > 0 && secondDirShadowArea > 0)
            return float4(0, 0.7, 1, 1); // Cyan: both directionals
        else if (dirShadowArea > 0)
            return float4(0, 0, 1, 1); // Blue: shadowed by directional 1
        else if (spotShadowArea > 0)
            return float4(1, 0, 0, 1); // Red: shadowed by spot
        else if (secondDirShadowArea > 0)
            return float4(0, 1, 0, 1); // Green: shadowed by directional 2
        else
            return float4(1, 1, 1, 1); // Lit by all
    }
    else
    {
        // Optional: tint shadowed regions for visual feedback
        float3 shadowTint = float3(1, 1, 1);
        if (dirShadowArea > 0 && spotShadowArea > 0 && secondDirShadowArea > 0)
            shadowTint = float3(0, 0, 0); // Black
        else if (dirShadowArea > 0 && secondDirShadowArea > 0)
            shadowTint = float3(0, 1, 1); // Cyan
        else if (dirShadowArea > 0)
            shadowTint = float3(0.7, 0.7, 1.0); // Blue
        else if (spotShadowArea > 0)
            shadowTint = float3(1.0, 0.7, 0.7); // Red
        else if (secondDirShadowArea > 0)
            shadowTint = float3(0.7, 1.0, 0.7); // Green
        // Lit by all: white

        return float4(colour.rgb * shadowTint, 1);
    }
}