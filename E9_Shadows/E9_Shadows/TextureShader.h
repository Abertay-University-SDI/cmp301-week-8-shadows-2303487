/**
 * TextureShader.h
 * ---------------
 * Defines the TextureShader class, which encapsulates a basic texture mapping shader for DirectX 11.
 * Handles shader initialization, parameter setting, and resource management for rendering textured geometry.
 * Inherits from BaseShader to reuse common shader functionality and resources.
 */

#pragma once

#include "BaseShader.h"
#include <DirectXMath.h>

using namespace DirectX;

// TextureShader: Handles the setup and use of a basic texture-mapping shader.
class TextureShader : public BaseShader
{
public:
	// Constructor: Initializes the shader with the given device and window handle.
	TextureShader(ID3D11Device* device, HWND hwnd);

	// Destructor: Releases allocated resources.
	~TextureShader();

	// Sets shader parameters before drawing.
	void setShaderParameters(ID3D11DeviceContext* deviceContext,
		const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
		ID3D11ShaderResourceView* texture);

private:
	// Loads and initializes shaders and their required buffers/samplers.
	void initShader(const wchar_t* vs, const wchar_t* ps);

	ID3D11Buffer* matrixBuffer = nullptr;          // Constant buffer for transformation matrices.
	ID3D11SamplerState* sampleState = nullptr;     // Sampler state for texture sampling.
};