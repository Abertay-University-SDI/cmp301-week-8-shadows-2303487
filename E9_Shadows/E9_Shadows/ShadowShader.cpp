// texture shader.cpp
#include "shadowshader.h"

ShadowShader::ShadowShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"shadow_vs.cso", L"shadow_ps.cso");
}

ShadowShader::~ShadowShader()
{
	if (sampleState) { sampleState->Release(); sampleState = 0; }
	if (matrixBuffer) { matrixBuffer->Release(); matrixBuffer = 0; }
	if (layout) { layout->Release(); layout = 0; }
	if (lightBuffer) { lightBuffer->Release(); lightBuffer = 0; }
	BaseShader::~BaseShader();
}

void ShadowShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;

	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	memset(samplerDesc.BorderColor, 0, sizeof(float) * 4);
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	renderer->CreateSamplerState(&samplerDesc, &sampleStateShadow);

	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);
}

void ShadowShader::setShaderParameters(
	ID3D11DeviceContext* deviceContext,
	const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
	ID3D11ShaderResourceView* texture,
	ID3D11ShaderResourceView* dirDepthMap,
	ID3D11ShaderResourceView* spotDepthMap,
	Light* dirLight,
	Light* spotLight,
	float spotCutoffDegrees,
	float spotExponent)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	LightBufferType* lightPtr;

	// Transpose all matrices for shader
	XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
	XMMATRIX tview = XMMatrixTranspose(viewMatrix);
	XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);
	XMMATRIX tDirLightView = XMMatrixTranspose(dirLight->getViewMatrix());
	XMMATRIX tDirLightProj = XMMatrixTranspose(dirLight->getOrthoMatrix());
	XMMATRIX tSpotLightView = XMMatrixTranspose(spotLight->getViewMatrix());
	XMMATRIX tSpotLightProj = XMMatrixTranspose(spotLight->getProjectionMatrix()); // Perspective!

	// --- Matrix buffer (b0) ---
	deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	dataPtr->dirLightView = tDirLightView;
	dataPtr->dirLightProj = tDirLightProj;
	dataPtr->spotLightView = tSpotLightView;
	dataPtr->spotLightProj = tSpotLightProj;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &matrixBuffer);

	// --- Light buffer (b1) ---
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;
	// Directional
	lightPtr->dirAmbient = dirLight->getAmbientColour();
	lightPtr->dirDiffuse = dirLight->getDiffuseColour();
	lightPtr->dirDirection = dirLight->getDirection();
	// Spot
	lightPtr->spotAmbient = spotLight->getAmbientColour();
	lightPtr->spotDiffuse = spotLight->getDiffuseColour();
	lightPtr->spotDirection = spotLight->getDirection();
	lightPtr->spotCutoff = spotCutoffDegrees; // Pass in degrees or cos(degrees) as needed
	lightPtr->spotPosition = spotLight->getPosition();
	lightPtr->spotExponent = spotExponent;
	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &lightBuffer);

	// --- Resource bindings ---
	deviceContext->PSSetShaderResources(0, 1, &texture);
	deviceContext->PSSetShaderResources(1, 1, &dirDepthMap);
	deviceContext->PSSetShaderResources(2, 1, &spotDepthMap);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
	deviceContext->PSSetSamplers(1, 1, &sampleStateShadow);
}