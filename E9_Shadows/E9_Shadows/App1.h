/**
 * App1.h
 * ------
 * Declares the App1 class, the main application entry point for rendering a scene with dual shadow lights.
 * Sets up all geometry, shaders, lights, and shadow maps. Handles initialization, frame update, and rendering passes
 * (depth, shadow, and final scene render), as well as basic GUI for real-time interaction and debugging.
 * Inherits from BaseApplication for core application lifecycle management.
 */

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

	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);
	bool frame() override;

protected:
	bool render() override;

	bool shadowDebugMode = false;
	void depthPass();
	void secondDirectionalDepthPass();      // <-- ADDED
	void createPostProcessRenderTarget(int width, int height);
	void updateSpotLight();
	void spotDepthPass();
	void finalPass();
	void gui();

private:
	PlaneMesh* mesh = nullptr;
	CubeMesh* cubeMesh = nullptr;
	SphereMesh* sphereMesh = nullptr;
	AModel* model = nullptr;

	TextureShader* textureShader = nullptr;
	ShadowShader* shadowShader = nullptr;
	DepthShader* depthShader = nullptr;

	Light* light = nullptr;
	Light* spotLight = nullptr;
	Light* secondDirectionalLight = nullptr;    // <-- ADDED

	ShadowMap* shadowMap = nullptr;
	ShadowMap* spotShadowMap = nullptr;
	ShadowMap* secondShadowMap = nullptr;       // <-- ADDED

	float spotCutoffDegrees = 60.0f;
	float spotExponent = 8.0f;
	XMMATRIX spotLightProjMatrix;

	float teapotAngle = 0.0f;

	bool wireframeToggle = false;
	bool sobelToggle = true;

	float heightScale = 8.0f;
	float prevHeightScale = 8.0f;

	ID3D11RasterizerState* shadowRasterState = nullptr;

	ID3D11Texture2D* postProcessTexture = nullptr;
	ID3D11RenderTargetView* postProcessRTV = nullptr;
	ID3D11ShaderResourceView* postProcessSRV = nullptr;
	FullscreenQuadMesh* fullscreenQuad = nullptr;
	PostProcessShader* postProcessShader = nullptr;

	int postProcessWidth = 0;
	int postProcessHeight = 0;
	int terrainResolution = 300;
	int prevTerrainResolution = 300;

	float spotLightX = 50.0f;
	float spotLightY = 30.0f;
	float spotLightZ = 10.0f;
	float spotFov = 70.0f;
	float spotNear = 1.0f;
	float spotFar = 100.0f;

	float spotShadowBias = 0.0005f;
};