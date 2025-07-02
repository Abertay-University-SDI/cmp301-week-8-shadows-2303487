#pragma once

#include "BaseShader.h"
#include <DirectXMath.h>

using namespace DirectX;

class TextureShader : public BaseShader
{
public:
	TextureShader(ID3D11Device* device, HWND hwnd);
	~TextureShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext,
		const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
		ID3D11ShaderResourceView* texture);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

	ID3D11Buffer* matrixBuffer = nullptr;
	ID3D11SamplerState* sampleState = nullptr;
};

