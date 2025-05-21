// App1.cpp
// Lab 1 example, simple coloured triangle mesh
#include "App1.h"


App1::App1()
{
	
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Create Mesh object and shader object
	mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext());
	model = new AModel(renderer->getDevice(), "res/teapot.obj");
	textureMgr->loadTexture(L"brick", L"res/brick1.dds");
	sphere = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());
	cube = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());

	// initial shaders
	textureShader = new TextureShader(renderer->getDevice(), hwnd);
	depthShader = new DepthShader(renderer->getDevice(), hwnd);
	shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

	// Variables for defining shadow map
	int shadowmapWidth = 2048;
	int shadowmapHeight = 2048;
	int sceneWidth = 100;
	int sceneHeight = 100;

	// This is your shadow map
	shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
	shadowMap2 = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);

	// Configure directional light
	light = new Light();
	light2 = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(0.0f, -0.7f, 0.7f);
	light->setPosition(0.f, 0.f, -10.f);
	light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);
	//light->generateOrthoMatrix(30.0f, 30.0f, 1.0f, 50.0f);
	//light->generateOrthoMatrix(150.0f, 150.0f, 0.1f, 300.0f);
	
	//float fov = 45.0f;
	//float aspectRatio = (float)screenWidth / (float)screenHeight;


}

App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
	if (sphere) { delete sphere; sphere = nullptr; }
	if (cube) { delete cube; cube = nullptr; }
	if (mesh) { delete mesh; mesh = nullptr; }
	if (model) { delete model; model = nullptr; }
	if (textureShader) { delete textureShader; textureShader = nullptr; }
	if (depthShader) { delete depthShader; depthShader = nullptr; }
	if (shadowShader) { delete shadowShader; shadowShader = nullptr; }
	if (shadowMap) { delete shadowMap; shadowMap = nullptr; }
	if (light) { delete light; light = nullptr; }
	if (light2) { delete light2; light2 = nullptr; }
	if (shadowMap2) { delete shadowMap2; shadowMap2 = nullptr; }
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}
	
	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::render()
{

	// Perform depth pass
	depthPass();
	// Render scene
	finalPass();

	return true;
}

void App1::renderDepthScene(const XMMATRIX& lightView, const XMMATRIX& lightProj)
{
	XMMATRIX worldMatrix;

	// Floor
	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
	mesh->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightView, lightProj);
	depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// Rotating model
	static float rot = 0.0f;
	rot += timer->getTime();
	XMMATRIX modelMatrix = XMMatrixRotationY(rot);
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(0.f, 7.f, 5.f));
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(0.5f, 0.5f, 0.5f));
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), modelMatrix, lightView, lightProj);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Sphere
	worldMatrix = XMMatrixTranslation(-10.f, 5.f, 0.f);
	sphere->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightView, lightProj);
	depthShader->render(renderer->getDeviceContext(), sphere->getIndexCount());

	// Cube
	worldMatrix = XMMatrixTranslation(10.f, 5.f, 0.f);
	cube->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightView, lightProj);
	depthShader->render(renderer->getDeviceContext(), cube->getIndexCount());
}


void App1::depthPass()
{
	// Set the render target to be the render to texture.
	shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());

	// get the world, view, and projection matrices from the camera and d3d objects.
	light->generateViewMatrix();
	XMMATRIX lightViewMatrix1 = light->getViewMatrix();
	XMMATRIX lightProjMatrix1 = light->getOrthoMatrix();

	renderDepthScene(lightViewMatrix1, lightProjMatrix1);

	shadowMap2->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
	light2->generateViewMatrix();

	XMMATRIX lightViewMatrix2 = light2->getViewMatrix();
	XMMATRIX lightProjMatrix2 = light2->getOrthoMatrix();

	renderDepthScene(lightViewMatrix2, lightProjMatrix2);

	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();

	XMMATRIX worldMatrix = renderer->getWorldMatrix();

	static float rot = 0.0f;
	rot += timer->getTime();  // Use same time delta
	XMMATRIX modelMatrix = XMMatrixRotationY(rot);
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(0.f, 7.f, 5.f));
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(0.5f, 0.5f, 0.5f));

	// Render model
	model->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), modelMatrix, lightViewMatrix1, lightProjMatrix1);
	depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

	//Render Sphere and Cube
	worldMatrix = XMMatrixTranslation(-10.f, 5.f, 0.f);
	sphere->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix1, lightProjMatrix1);
	depthShader->render(renderer->getDeviceContext(), sphere->getIndexCount());

	worldMatrix = XMMatrixTranslation(10.f, 5.f, 0.f);
	cube->sendData(renderer->getDeviceContext());
	depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix1, lightProjMatrix1);
	depthShader->render(renderer->getDeviceContext(), cube->getIndexCount());

	// Set back buffer as render target and reset view port.
	renderer->setBackBufferRenderTarget();
	renderer->resetViewport();
}

void App1::finalPass()
{
	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	camera->update();

	// get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	XMMATRIX worldMatrix = renderer->getWorldMatrix();
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

	worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);

	// Render floor
	mesh->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), shadowMap->getDepthMapSRV(), shadowMap2->getDepthMapSRV(), light, light2);
	shadowShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

	// Render model and animate
	static float rot = 0.0f;
	rot += timer->getTime();  // time since last frame
	XMMATRIX modelMatrix = XMMatrixRotationY(rot);
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(0.f, 7.f, 5.f));
	modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(0.5f, 0.5f, 0.5f));

	model->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), shadowMap->getDepthMapSRV(), shadowMap2->getDepthMapSRV(), light, light2);
	shadowShader->render(renderer->getDeviceContext(), model->getIndexCount());

	// Sphere
	worldMatrix = XMMatrixTranslation(-10.f, 5.f, 0.f);
	sphere->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), shadowMap->getDepthMapSRV(), shadowMap2->getDepthMapSRV(), light, light2);
	shadowShader->render(renderer->getDeviceContext(), sphere->getIndexCount());

	// Cube
	worldMatrix = XMMatrixTranslation(10.f, 5.f, 0.f);
	cube->sendData(renderer->getDeviceContext());
	shadowShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture(L"brick"), shadowMap->getDepthMapSRV(), shadowMap2->getDepthMapSRV(), light, light2);
	shadowShader->render(renderer->getDeviceContext(), cube->getIndexCount());

	gui();
	renderer->endScene();
}



void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI
	ImGui::Text("FPS: %.2f", timer->getFPS());
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);

	ImGui::SliderFloat3("Light Dir", &lightDir.x, -1.0f, 1.0f);
	light->setDirection(lightDir.x, lightDir.y, lightDir.z);
	ImGui::SliderFloat3("Light 2 Dir", &lightDir2.x, -1.0f, 1.0f);
	light2->setDirection(lightDir2.x, lightDir2.y, lightDir2.z);

	static float orthoWidth = 100.0f;
	static float orthoHeight = 100.0f;
	static float nearPlane = 0.1f;
	static float farPlane = 100.0f;

	if (ImGui::SliderFloat("Ortho Width", &orthoWidth, 10.0f, 200.0f), ImGui::SliderFloat("Ortho Height", &orthoHeight, 10.0f, 200.0f ), ImGui::SliderFloat("Near Plane", &nearPlane, 0.01f, 10.0f), ImGui::SliderFloat("Far Plane", &farPlane, 10.0f, 500.0f))
	{
		light->generateOrthoMatrix(orthoWidth, orthoHeight, nearPlane, farPlane);
	}

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

