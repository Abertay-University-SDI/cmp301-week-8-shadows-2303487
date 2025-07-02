/**
 * TextureShader.cpp
 * -----------------
 * Implements the TextureShader class, responsible for initializing, configuring, and managing
 * a simple texture-mapping shader pipeline in DirectX 11. Handles the setup of constant buffers
 * and sampler states, as well as the passing of transformation matrices and textures to the GPU.
 */

#include "TextureShader.h"

 // Constructor: Initialize shader and resources.
TextureShader::TextureShader(ID3D11Device* device, HWND hwnd)
	: BaseShader(device, hwnd)
{
	initShader(L"texture_vs.cso", L"texture_ps.cso");
}

// Destructor: Release allocated DirectX resources.
TextureShader::~TextureShader()
{
	if (sampleState) { sampleState->Release(); sampleState = nullptr; }
	if (matrixBuffer) { matrixBuffer->Release(); matrixBuffer = nullptr; }
	if (layout) { layout->Release(); layout = nullptr; }
	// BaseShader destructor handles further cleanup.
}

// Initialize shaders, constant buffer, and sampler.
void TextureShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc = {};
	D3D11_SAMPLER_DESC samplerDesc = {};

	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup matrix buffer for passing transformation matrices to the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	renderer->CreateBuffer(&matrixBufferDesc, nullptr, &matrixBuffer);

	// Setup texture sampler for pixel shader.
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}

// Set shader parameters (matrices & texture) before rendering.
void TextureShader::setShaderParameters(
	ID3D11DeviceContext* deviceContext,
	const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
	ID3D11ShaderResourceView* texture)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr = nullptr;

	// Transpose matrices for HLSL.
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

	// Set matrices for vertex shader.
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// Set texture and sampler for pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
}