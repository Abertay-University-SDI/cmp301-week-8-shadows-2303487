#pragma once
#include "DXF.h"

class PostProcessShader : public BaseShader
{
public:
    PostProcessShader(ID3D11Device* device, HWND hwnd);
    ~PostProcessShader();

    void setShaderParameters(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* sceneSRV,
        XMFLOAT2 texelSize);

    void render(ID3D11DeviceContext* deviceContext, int indexCount);

private:
    void initShader(const wchar_t* vs, const wchar_t* ps);

    ID3D11Buffer* screenSizeBuffer = nullptr;
};