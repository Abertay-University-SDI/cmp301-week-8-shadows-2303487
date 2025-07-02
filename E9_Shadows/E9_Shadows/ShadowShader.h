// Light shader.h
// Basic single light shader setup
#ifndef _SHADOWSHADER_H_
#define _SHADOWSHADER_H_

#include "DXF.h"
using namespace std;
using namespace DirectX;

class ShadowShader : public BaseShader
{
private:
	struct MatrixBufferType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;
		XMMATRIX dirLightView;
		XMMATRIX dirLightProj;
		XMMATRIX spotLightView;
		XMMATRIX spotLightProj;
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
		float spotCutoff;    // In radians or cos(cutoff)
		XMFLOAT3 spotPosition;
		float spotExponent;
	};

public:
	ShadowShader(ID3D11Device* device, HWND hwnd);
	~ShadowShader();

	void setShaderParameters(
		ID3D11DeviceContext* deviceContext,
		const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
		ID3D11ShaderResourceView* texture,
		ID3D11ShaderResourceView* dirDepthMap,
		ID3D11ShaderResourceView* spotDepthMap,
		Light* dirLight,
		Light* spotLight,
		float spotCutoffDegrees,
		float spotExponent
	);

private:
	void initShader(const wchar_t* vs, const wchar_t* ps);

	ID3D11Buffer* matrixBuffer;
	ID3D11SamplerState* sampleState;
	ID3D11SamplerState* sampleStateShadow;
	ID3D11Buffer* lightBuffer;
};

#endif