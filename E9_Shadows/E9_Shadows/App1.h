/**
 * App1.h
 * ------
 * Declares the App1 class, the main application entry point for rendering a scene with dual shadow lights.
 * Sets up all geometry, shaders, lights, and shadow maps. Handles initialization, frame update, and rendering passes
 * (depth, shadow, and final scene render), as well as basic GUI for real-time interaction and debugging.
 * Inherits from BaseApplication for core application lifecycle management.
 */

#pragma once

#include "DXF.h"
#include "TextureShader.h"
#include "ShadowShader.h"
#include "DepthShader.h"
#include "FullscreenQuadMesh.h"
#include "PostProcessShader.h"
#include "BaseApplication.h"
#include "D3D.h"

class App1 : public BaseApplication
{
public:
	App1();
	~App1();

	// Initialize application resources, scene, shaders, lights, and shadow maps
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	// Main per-frame update
	bool frame() override;

protected:
	// Main render routine, executes all rendering passes
	bool render() override;

	// Render scene from directional light's perspective for shadow mapping
	void depthPass();

	void createPostProcessRenderTarget(int width, int height);

	// Render scene from spotlight's perspective for shadow mapping
	void spotDepthPass();

	// Render the final scene with full lighting and shadows
	void finalPass();

	// Draw the ImGui interface and debug overlays
	void gui();

private:
	// Scene meshes and models
	PlaneMesh* mesh = nullptr;
	CubeMesh* cubeMesh = nullptr;
	SphereMesh* sphereMesh = nullptr;
	AModel* model = nullptr;

	// Shaders
	TextureShader* textureShader = nullptr;
	ShadowShader* shadowShader = nullptr;
	DepthShader* depthShader = nullptr;

	// Lights
	Light* light = nullptr;
	Light* spotLight = nullptr;

	// Shadow map render targets
	ShadowMap* shadowMap = nullptr;
	ShadowMap* spotShadowMap = nullptr;

	// Spotlight parameters
	float spotCutoffDegrees = 60.0f;
	float spotExponent = 8.0f;
	XMMATRIX spotLightProjMatrix;

	// Animation state
	float teapotAngle = 0.0f;

	// Wireframe rendering toggle
	bool wireframeToggle = false;

	float heightScale = 8.0f;
	float prevHeightScale = 8.0f; 

	ID3D11RasterizerState* shadowRasterState = nullptr;

	// Post-processing resources
	ID3D11Texture2D* postProcessTexture = nullptr;
	ID3D11RenderTargetView* postProcessRTV = nullptr;
	ID3D11ShaderResourceView* postProcessSRV = nullptr;
	FullscreenQuadMesh* fullscreenQuad = nullptr;
	PostProcessShader* postProcessShader = nullptr;

	int postProcessWidth = 0;
	int postProcessHeight = 0;

};