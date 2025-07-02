#include "PostProcessShader.h"

struct ScreenSizeBufferType
{
    XMFLOAT2 texelSize;
    XMFLOAT2 padding;
};

PostProcessShader::PostProcessShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
    initShader(L"SobelPostProcessVS.cso", L"SobelPostProcessPS.cso");
}

PostProcessShader::~PostProcessShader()
{
    if (screenSizeBuffer) { screenSizeBuffer->Release(); screenSizeBuffer = nullptr; }
    // No need to call BaseShader destructor explicitly; it will be called automatically
}

void PostProcessShader::initShader(const wchar_t* vs, const wchar_t* ps)
{
    loadVertexShader(vs);
    loadPixelShader(ps);

    // Screen size constant buffer
    D3D11_BUFFER_DESC screenSizeDesc = {};
    screenSizeDesc.Usage = D3D11_USAGE_DYNAMIC;
    screenSizeDesc.ByteWidth = sizeof(ScreenSizeBufferType);
    screenSizeDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    screenSizeDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    screenSizeDesc.MiscFlags = 0;
    screenSizeDesc.StructureByteStride = 0;
    renderer->CreateBuffer(&screenSizeDesc, NULL, &screenSizeBuffer);
}

void PostProcessShader::setShaderParameters(
    ID3D11DeviceContext* deviceContext,
    ID3D11ShaderResourceView* sceneSRV,
    XMFLOAT2 texelSize)
{
    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    deviceContext->Map(screenSizeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    ScreenSizeBufferType* dataPtr = (ScreenSizeBufferType*)mappedResource.pData;
    dataPtr->texelSize = texelSize;
    dataPtr->padding = XMFLOAT2(0, 0);
    deviceContext->Unmap(screenSizeBuffer, 0);

    deviceContext->VSSetConstantBuffers(0, 1, &screenSizeBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &screenSizeBuffer);

    deviceContext->PSSetShaderResources(0, 1, &sceneSRV);

    // Use default sampler
    deviceContext->PSSetSamplers(0, 1, &sampleState);
}

void PostProcessShader::render(ID3D11DeviceContext* deviceContext, int indexCount)
{
    BaseShader::render(deviceContext, indexCount);
}