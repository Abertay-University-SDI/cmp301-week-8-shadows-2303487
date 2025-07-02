/**
 * DepthShader.cpp
 * ---------------
 * Implements the DepthShader class, which provides initialization, configuration, and
 * resource management for a basic depth rendering shader in DirectX 11. Used mainly for
 * depth-map generation in advanced rendering techniques such as shadow mapping.
 */

#include "DepthShader.h"

 // Constructor: Initialize shader resources.
DepthShader::DepthShader(ID3D11Device* device, HWND hwnd)
    : BaseShader(device, hwnd)
{
    initShader(L"depth_vs.cso", L"depth_ps.cso");
}

// Destructor: Release DirectX resources.
DepthShader::~DepthShader()
{
    if (matrixBuffer) { matrixBuffer->Release(); matrixBuffer = nullptr; }
    if (layout) { layout->Release(); layout = nullptr; }
    // BaseShader destructor handles further cleanup.
}

// Initialize shaders and constant buffer.
void DepthShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
    D3D11_BUFFER_DESC matrixBufferDesc = {};
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    loadVertexShader(vsFilename);
    loadPixelShader(psFilename);

    renderer->CreateBuffer(&matrixBufferDesc, nullptr, &matrixBuffer);
}

// Set shader parameters (matrices) before rendering.
void DepthShader::setShaderParameters(ID3D11DeviceContext* deviceContext,
    const XMMATRIX& worldMatrix,
    const XMMATRIX& viewMatrix,
    const XMMATRIX& projectionMatrix)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    MatrixBufferType* dataPtr;

    // Transpose matrices for HLSL compatibility.
    XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
    XMMATRIX tview = XMMatrixTranspose(viewMatrix);
    XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

    // Map buffer and copy matrices.
    deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    dataPtr = (MatrixBufferType*)mappedResource.pData;
    dataPtr->world = tworld;
    dataPtr->view = tview;
    dataPtr->projection = tproj;
    deviceContext->Unmap(matrixBuffer, 0);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE; // No culling for accurate self-shadowing
    rasterDesc.DepthBias = 100; // Try 50–500, tweak as needed
    rasterDesc.SlopeScaledDepthBias = 2.0f; // Try 1.0–3.0
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;

    // Set matrices for vertex shader.
    deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);
}