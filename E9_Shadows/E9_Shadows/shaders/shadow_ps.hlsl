
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture : register(t1);
Texture2D depthMapTexture2 : register(t2);

SamplerState diffuseSampler  : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b0)
{
    float4 ambient1;
    float4 diffuse1;
    float3 direction1;
    float padding1; 

    float4 ambient2;
    float4 diffuse2;
    float3 direction2;
    float padding2;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
    float4 lightViewPos1 : TEXCOORD1;
    float4 lightViewPos2 : TEXCOORD2;
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDir, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normalize(normal), normalize(-lightDir)));
    return saturate(diffuse * intensity);
}


// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    // Sample the shadow map (get depth of geometry)
    float depthValue = sMap.Sample(shadowSampler, uv).r;
	// Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    lightDepthValue -= bias;

	// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue < depthValue)
    {
        return false;
    }
    return true;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5f, -0.5f);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float4 main(InputType input) : SV_TARGET
{
    float shadowBias = 0.005f;
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

    float2 texCoord1 = getProjectiveCoords(input.lightViewPos1);
    float2 texCoord2 = getProjectiveCoords(input.lightViewPos2);

    float4 totalLighting = float4(0, 0, 0, 1);

    // Light 1
    if (hasDepthData(texCoord1))
    {
        if (!isInShadow(depthMapTexture, texCoord1, input.lightViewPos1, shadowBias))
        {
            totalLighting += calculateLighting(direction1, input.normal, diffuse1);
        }
    }

    // Light 2
    if (hasDepthData(texCoord2))
    {
        if (!isInShadow(depthMapTexture2, texCoord2, input.lightViewPos2, shadowBias))
        {
            totalLighting += calculateLighting(direction2, input.normal, diffuse2);
        }
    }

    // Combine with ambient
    totalLighting += (ambient1 + ambient2);

    return saturate(totalLighting) * textureColour;
}