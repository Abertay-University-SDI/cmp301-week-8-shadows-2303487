Texture2D shaderTexture : register(t0);
Texture2D dirShadowMapTexture : register(t1);
Texture2D spotShadowMapTexture : register(t2);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b1)
{
    float4 dirAmbient;
    float4 dirDiffuse;
    float3 dirDirection;
    float  pad0;
    float4 spotAmbient;
    float4 spotDiffuse;
    float3 spotDirection;
    float  spotCutoff; // cos(cutoff angle)
    float3 spotPosition;
    float  spotExponent;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 dirLightViewPos : TEXCOORD1;
    float4 spotLightViewPos : TEXCOORD2;
    float4 worldPos : TEXCOORD3;
};

float4 calculateLighting(float3 lightDirection, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(diffuse * intensity);
    return colour;
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

float2 getProjectiveCoords(float4 lightViewPosition)
{
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float4 main(OutputType input) : SV_TARGET
{
    float shadowMapBias = 0.005f;
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

    // Directional Light
    float2 dirTexCoord = getProjectiveCoords(input.dirLightViewPos);
    float dirShadow = 1.0f;
    if (hasDepthData(dirTexCoord))
        dirShadow = !isInShadow(dirShadowMapTexture, dirTexCoord, input.dirLightViewPos, shadowMapBias);

    float4 dirLightCol = calculateLighting(-dirDirection, input.normal, dirDiffuse) * dirShadow;

    // Spot Light
    float2 spotTexCoord = getProjectiveCoords(input.spotLightViewPos);
    float spotShadow = 1.0f;
    if (hasDepthData(spotTexCoord))
        spotShadow = !isInShadow(spotShadowMapTexture, spotTexCoord, input.spotLightViewPos, shadowMapBias);

    float3 lightToPixel = normalize(input.worldPos.xyz - spotPosition);

    // Fix 1: Use correct direction for spotlight (should be from spotPosition to worldPos, not the reverse)
    float spotFactor = dot(normalize(spotDirection), normalize(input.worldPos.xyz - spotPosition));
    // Or, equivalently: float spotFactor = dot(spotDirection, -lightToPixel);

    // Fix 2: Clamp spotFactor, and ensure cutoff is in cosine
    float spotLightVal = 0;
    if (spotFactor > spotCutoff)
        spotLightVal = pow(spotFactor, spotExponent);

    float4 spotLightCol = calculateLighting(normalize(spotDirection), input.normal, spotDiffuse) * spotLightVal * spotShadow;

    // Combine lights and ambient
    float4 colour = dirAmbient + dirLightCol + spotAmbient + spotLightCol;
    return saturate(colour) * textureColour;
}