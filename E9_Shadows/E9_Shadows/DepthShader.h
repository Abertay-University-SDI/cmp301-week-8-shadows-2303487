/**
 * DepthShader.h
 * -------------
 * Defines the DepthShader class for DirectX 11, which is responsible for rendering to a depth map.
 * This shader is typically used for shadow mapping or depth-based effects, outputting linearized
 * or projected depth from the camera's perspective. Inherits from BaseShader for common functionality.
 */

#pragma once

#include "DXF.h"

 // DepthShader: Handles the setup and use of a shader for depth rendering.
class DepthShader : public BaseShader
{
public:
    // Constructor: Initializes the shader with device and window handle.
    DepthShader(ID3D11Device* device, HWND hwnd);

    // Destructor: Releases allocated DirectX resources.
    ~DepthShader();

    // Sets shader parameters before drawing (matrices only, no textures).
    void setShaderParameters(ID3D11DeviceContext* deviceContext,
        const XMMATRIX& world,
        const XMMATRIX& view,
        const XMMATRIX& projection);

private:
    // Loads and initializes shaders and the required matrix buffer.
    void initShader(const wchar_t* vs, const wchar_t* ps);

    ID3D11Buffer* matrixBuffer = nullptr; // Constant buffer for transformation matrices.
};