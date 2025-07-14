/**
 * ShadowShader.h
 * --------------
 * Defines the ShadowShader class for DirectX 11.
 * This shader implements shadow mapping with support for both directional and spot lights.
 * It manages all resources and state needed for rendering shadows, including multiple constant buffers and samplers.
 * Used for advanced lighting effects such as hard/soft shadows and combined multi-light illumination.
 */

#pragma once

#include "BaseShader.h"
#include "Light.h"
#include <DirectXMath.h>

using namespace DirectX;

class ShadowShader : public BaseShader
{
public:
    ShadowShader(ID3D11Device* device, HWND hwnd);
    ~ShadowShader();

    void setShaderParameters(
        ID3D11DeviceContext* deviceContext,
        const XMMATRIX& world,
        const XMMATRIX& view,
        const XMMATRIX& projection,
        ID3D11ShaderResourceView* texture,
        ID3D11ShaderResourceView* dirDepthMap,
        ID3D11ShaderResourceView* spotDepthMap,
        ID3D11ShaderResourceView* secondDirDepthMap,
        Light* dirLight,
        Light* spotLight,
        Light* secondDirLight,
        float spotCutoffDegrees,
        float spotExponent,
        bool shadowDebug,
        float spotShadowBias
    );

private:
    void initShader(const wchar_t* vs, const wchar_t* ps);

    struct MatrixBufferType
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX dirLightView;
        XMMATRIX dirLightProj;
        XMMATRIX spotLightView;
        XMMATRIX spotLightProj;
        XMMATRIX secondDirLightView;
        XMMATRIX secondDirLightProj;
    };

    struct LightBufferType
    {
        XMFLOAT4 dirAmbient;
        XMFLOAT4 dirDiffuse;
        XMFLOAT3 dirDirection;
        float pad0;
        XMFLOAT4 spotAmbient;
        XMFLOAT4 spotDiffuse;
        XMFLOAT3 spotDirection;
        float spotCutoff;
        XMFLOAT3 spotPosition;
        float spotExponent;
        XMFLOAT4 secondDirAmbient;
        XMFLOAT4 secondDirDiffuse;
        XMFLOAT3 secondDirDirection;
        float pad1;
        int shadowDebug;
        float pad2[3]; // alignment
    };

    ID3D11Buffer* matrixBuffer = nullptr;
    ID3D11SamplerState* sampleState = nullptr;
    ID3D11SamplerState* sampleStateShadow = nullptr;
    ID3D11Buffer* lightBuffer = nullptr;
};