// App1.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "TextureShader.h"
#include "ShadowShader.h"
#include "DepthShader.h"
#include "SphereMesh.h"
#include "CubeMesh.h"

class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool render();
	void renderDepthScene(const XMMATRIX& lightView, const XMMATRIX& lightProj);
	void depthPass();
	void finalPass();
	void gui();

private:
	TextureShader* textureShader;
	PlaneMesh* mesh;

	Light* light;
	Light* light2;
	AModel* model;
	ShadowShader* shadowShader;
	DepthShader* depthShader;

	SphereMesh* sphere;
	CubeMesh* cube;

	ShadowMap* shadowMap;
	ShadowMap* shadowMap2;

	XMFLOAT3 lightDir = XMFLOAT3(0.0f, -0.7f, 0.7f);
	XMFLOAT3 lightDir2 = XMFLOAT3(0.0f, -0.7f, -0.7f);
};

#endif