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
};

OutputType main(InputType input)
{
    OutputType output;

    // Transform vertex to render space
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
    return output;
}