// Application.h
#pragma once

#include "DXF.h"
#include "TextureShader.h"
#include "ShadowShader.h"
#include "DepthShader.h"

class App1 : public BaseApplication
{
public:
	App1();
	~App1();

	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame() override;

protected:
	bool render() override;
	void depthPass();
	void spotDepthPass();
	void finalPass();
	void gui();

private:
	// Meshes and models
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

	// Shadow maps
	ShadowMap* shadowMap = nullptr;
	ShadowMap* spotShadowMap = nullptr;

	// Spot light parameters
	float spotCutoffDegrees = 60.0f;
	float spotExponent = 8.0f;
	XMMATRIX spotLightProjMatrix;

	// Animation state
	float teapotAngle = 0.0f;

	// Wireframe toggle
	bool wireframeToggle = false;
};
