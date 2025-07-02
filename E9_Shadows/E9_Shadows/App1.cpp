/**
 * App1.cpp
 * --------
 * Implements the main application logic, scene setup, and rendering for a dual-light, dual-shadow demo.
 * Initializes all geometry, shaders, lights, and shadow maps, and executes three-pass rendering:
 * - depthPass: render scene from the directional light for shadow mapping
 * - spotDepthPass: render scene from the spotlight for shadow mapping
 * - finalPass: render scene with full lighting, texturing, and shadows
 * Also handles ImGui-based UI and real-time parameter adjustment.
 */

#include "App1.h"
#include "PlaneMesh.h"

App1::App1()
{
	// Initialize pointers to nullptr for safety
	mesh = nullptr;
	cubeMesh = nullptr;
	sphereMesh = nullptr;
	model = nullptr;
	textureShader = nullptr;
	shadowShader = nullptr;
	depthShader = nullptr;
	light = nullptr;
	spotLight = nullptr;
	shadowMap = nullptr;
	spotShadowMap = nullptr;
	shadowRasterState = nullptr;
	teapotAngle = 0.0f;
	wireframeToggle = false;
}

App1::~App1()
{
	// Release all heap-allocated resources
	delete mesh;
	delete cubeMesh;
	delete sphereMesh;
	delete model;
	delete textureShader;
	delete shadowShader;
	delete depthShader;
	delete light;
	delete spotLight;
	delete shadowMap;
	delete spotShadowMap;
	if (shadowRasterState) { shadowRasterState->Release(); shadowRasterState = nullptr; }
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Load meshes and models
	mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, 300);
	model = new AModel(renderer->getDevice(), "res/teapot.obj");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	cubeMesh = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
	sphereMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());

	// Create shaders
	textureShader = new TextureShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

	// Create rasterizer state for shadow mapping (CULL_NONE for depth passes only)
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 100; // Tweak as needed
	rasterDesc.SlopeScaledDepthBias = 2.0f; // Tweak as needed
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = TRUE;

	HRESULT hr = renderer->getDevice()->CreateRasterizerState(&rasterDesc, &shadowRasterState);
	// Check hr for errors!

	int shadowmapWidth = 1024;
	int shadowmapHeight = 1024;
	int sceneWidth = 100;
	int sceneHeight = 100;

	// Directional shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Directional light setup
	light = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(0.0f, -0.7f, 0.7f);
	light->setPosition(0.f, 0.f, -10.f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

	// Spot light setup
	spotLight = new Light();
	spotLight->setAmbientColour(0.2f, 0.2f, 0.2f, 1.0f);
	spotLight->setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f);
	spotLight->setPosition(0.0f, 30.0f, 0.0f);
	spotLight->setDirection(0.0f, -1.0f, 0.0f);

	spotCutoffDegrees = 60.0f;
	spotExponent = 8.0f;

	spotShadowMap = new ShadowMap(renderer->getDevice(), 512, 512);

	// Create a perspective projection for the spotlight
	spotLightProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1.0f, 1.0f, 100.0f);
}

bool App1::frame()
{
	// Animate teapot
	teapotAngle += 0.01f;
	if (teapotAngle > XM_2PI) teapotAngle -= XM_2PI;

	if (heightScale != prevHeightScale) {
		delete mesh;
		mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, 300);
		prevHeightScale = heightScale;
	}

	if (!BaseApplication::frame())
		return false;

	if (!render())
		return false;

	return true;
}

bool App1::render()
{
	depthPass();
	spotDepthPass();
	finalPass();
	return true;
}

// Render scene from the directional light's view for shadow mapping
void App1::depthPass()
{
	renderer->getDeviceContext()->RSSetState(shadowRasterState);

	shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
	light->generateViewMatrix();
	XMMATRIX lightViewMatrix = light->getViewMatrix();
	XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();
	XMMATRIX worldMatrix;

	// Floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// Teapot
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
	XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Cube
	XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
	worldMatrix = cubeScale * cubeTranslate;
	cubeMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

	// Sphere
	XMMATRIX sphereScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX sphereTranslate = XMMatrixTranslation(-20.f, 4.f, 0.f);
	worldMatrix = sphereScale * sphereTranslate;
	sphereMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
	depthShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
	renderer->getDeviceContext()->RSSetState(nullptr); // Restore default rasterizer state
}

// Render scene from the spotlight's view for shadow mapping
void App1::spotDepthPass()
{
	renderer->getDeviceContext()->RSSetState(shadowRasterState);

	spotShadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
	spotLight->generateViewMatrix();
	XMMATRIX spotViewMatrix = spotLight->getViewMatrix();
	XMMATRIX spotProjMatrix = spotLightProjMatrix;
	XMMATRIX worldMatrix;

	// Floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
	depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// Teapot
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
	XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Cube
	XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
	worldMatrix = cubeScale * cubeTranslate;
	cubeMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
	depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

	// Sphere
	XMMATRIX sphereScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX sphereTranslate = XMMatrixTranslation(-20.f, 4.f, 0.f);
	worldMatrix = sphereScale * sphereTranslate;
	sphereMesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
	depthShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
	renderer->getDeviceContext()->RSSetState(nullptr); // Restore default rasterizer state
}

// Final render pass using all shadow and lighting information
void App1::finalPass()
{
	// IMPORTANT: Use CULL_NONE so both front and back faces are rendered (shadows on both sides)
	renderer->getDeviceContext()->RSSetState(shadowRasterState);

	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	XMMATRIX worldMatrix;
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

	// Floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix, viewMatrix, projectionMatrix,
		textureMgr->getTexture(L"brick"),
		shadowMap->getDepthMapSRV(),
		spotShadowMap->getDepthMapSRV(),
		light,
		spotLight,
		cos(XMConvertToRadians(spotCutoffDegrees)),
		spotExponent
	);
	shadowShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// Teapot
	XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
	XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
	worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
	model->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix, viewMatrix, projectionMatrix,
		textureMgr->getTexture(L"brick"),
		shadowMap->getDepthMapSRV(),
		spotShadowMap->getDepthMapSRV(),
		light,
		spotLight,
		cos(XMConvertToRadians(spotCutoffDegrees)),
		spotExponent
	);
	shadowShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Cube
	XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
	worldMatrix = cubeScale * cubeTranslate;
	cubeMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix, viewMatrix, projectionMatrix,
		textureMgr->getTexture(L"brick"),
		shadowMap->getDepthMapSRV(),
		spotShadowMap->getDepthMapSRV(),
		light,
		spotLight,
		cos(XMConvertToRadians(spotCutoffDegrees)),
		spotExponent
	);
	shadowShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

	// Sphere
	XMMATRIX sphereScale = XMMatrixScaling(4.f, 4.f, 4.f);
	XMMATRIX sphereTranslate = XMMatrixTranslation(-20.f, 4.f, 0.f);
	worldMatrix = sphereScale * sphereTranslate;
	sphereMesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(
		renderer->getDeviceContext(),
		worldMatrix, viewMatrix, projectionMatrix,
		textureMgr->getTexture(L"brick"),
		shadowMap->getDepthMapSRV(),
		spotShadowMap->getDepthMapSRV(),
		light,
		spotLight,
		cos(XMConvertToRadians(spotCutoffDegrees)),
		spotExponent
	);
	shadowShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

	// Draw UI overlays
	gui();
	renderer->endScene();

	// Restore default rasterizer state after main pass (optional but recommended)
	renderer->getDeviceContext()->RSSetState(nullptr);
}

// Draw basic ImGui interface and wireframe toggle
void App1::gui()
{
	// Unbind any geometry, hull, or domain shaders for safety
	renderer->getDeviceContext()->GSSetShader(nullptr, nullptr, 0);
	renderer->getDeviceContext()->HSSetShader(nullptr, nullptr, 0);
	renderer->getDeviceContext()->DSSetShader(nullptr, nullptr, 0);

	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);
	ImGui::SliderFloat("Plane Height Scale", &heightScale, 1.0f, 100.0f);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}