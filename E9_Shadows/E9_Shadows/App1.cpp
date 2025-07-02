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

 // Constructor
App1::App1()
{
    // Initialize main pointers to nullptr
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
    fullscreenQuad = nullptr;
    postProcessShader = nullptr;
    postProcessTexture = nullptr;
    postProcessRTV = nullptr;
    postProcessSRV = nullptr;
    postProcessWidth = 0;
    postProcessHeight = 0;
}

// Destructor
App1::~App1()
{
    // Safe deletes/releases
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
    if (fullscreenQuad) { delete fullscreenQuad; fullscreenQuad = nullptr; }
    if (postProcessShader) { delete postProcessShader; postProcessShader = nullptr; }
    if (postProcessTexture) { postProcessTexture->Release(); postProcessTexture = nullptr; }
    if (postProcessRTV) { postProcessRTV->Release(); postProcessRTV = nullptr; }
    if (postProcessSRV) { postProcessSRV->Release(); postProcessSRV = nullptr; }
}

// Create post-processing render target (RTV/SRV/Texture)
void App1::createPostProcessRenderTarget(int width, int height)
{
    if (postProcessTexture) { postProcessTexture->Release(); postProcessTexture = nullptr; }
    if (postProcessRTV) { postProcessRTV->Release(); postProcessRTV = nullptr; }
    if (postProcessSRV) { postProcessSRV->Release(); postProcessSRV = nullptr; }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = renderer->getDevice()->CreateTexture2D(&texDesc, nullptr, &postProcessTexture);
    if (FAILED(hr)) throw std::runtime_error("Failed to create post-process texture!");

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    hr = renderer->getDevice()->CreateRenderTargetView(postProcessTexture, &rtvDesc, &postProcessRTV);
    if (FAILED(hr)) throw std::runtime_error("Failed to create post-process RTV!");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = renderer->getDevice()->CreateShaderResourceView(postProcessTexture, &srvDesc, &postProcessSRV);
    if (FAILED(hr)) throw std::runtime_error("Failed to create post-process SRV!");

    // Store current width/height
    postProcessWidth = width;
    postProcessHeight = height;
}

// Initialization
void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
    BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

    // Load meshes and models
    mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, 300);
    model = new AModel(renderer->getDevice(), "res/teapot.obj");
    textureMgr->loadTexture(L"brick", L"res/brick1.dds");
    cubeMesh = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
    sphereMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());

    // Shaders
    textureShader = new TextureShader(renderer->getDevice(), hwnd);
    depthShader = new DepthShader(renderer->getDevice(), hwnd);
    shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

    // Rasterizer state for shadow mapping
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 100; // Tweak as needed
    rasterDesc.SlopeScaledDepthBias = 2.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;

    HRESULT hr = renderer->getDevice()->CreateRasterizerState(&rasterDesc, &shadowRasterState);
    if (FAILED(hr)) throw std::runtime_error("Failed to create rasterizer state!");

    // Shadow maps
    int shadowmapWidth = 1024, shadowmapHeight = 1024;
    shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
    spotShadowMap = new ShadowMap(renderer->getDevice(), 512, 512);

    // Post-process objects
    fullscreenQuad = new FullscreenQuadMesh(renderer->getDevice(), renderer->getDeviceContext());
    postProcessShader = new PostProcessShader(renderer->getDevice(), hwnd);
    createPostProcessRenderTarget(screenWidth, screenHeight);
    // postProcessWidth/postProcessHeight are set in createPostProcessRenderTarget

    // Lights
    int sceneWidth = 100, sceneHeight = 100;
    light = new Light();
    light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
    light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
    light->setDirection(0.0f, -0.7f, 0.7f);
    light->setPosition(0.f, 0.f, -10.f);
    light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

    spotLight = new Light();
    spotLight->setAmbientColour(0.2f, 0.2f, 0.2f, 1.0f);
    spotLight->setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f);
    spotLight->setPosition(0.0f, 30.0f, 0.0f);
    spotLight->setDirection(0.0f, -1.0f, 0.0f);

    spotCutoffDegrees = 60.0f;
    spotExponent = 8.0f;

    // Spot light projection
    spotLightProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1.0f, 1.0f, 100.0f);
}

bool App1::frame()
{
    // Animate teapot
    teapotAngle += 0.01f;
    if (teapotAngle > XM_2PI) teapotAngle -= XM_2PI;

    // Heightmap live reload
    if (heightScale != prevHeightScale) {
        delete mesh;
        mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, 300);
        prevHeightScale = heightScale;
    }

    if (!BaseApplication::frame()) return false;
    if (!render()) return false;
    return true;
}

bool App1::render()
{
    depthPass();
    spotDepthPass();
    finalPass();
    return true;
}

// Depth pass (directional)
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
    renderer->getDeviceContext()->RSSetState(nullptr);
}

// Depth pass (spotlight)
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
    renderer->getDeviceContext()->RSSetState(nullptr);
}

// Final pass: render lit scene to post-process target, then Sobel, then UI
void App1::finalPass()
{
    // 1. Render scene to post-process render target
    renderer->getDeviceContext()->RSSetState(shadowRasterState);
    renderer->getDeviceContext()->OMSetRenderTargets(1, &postProcessRTV, renderer->getDepthStencilViewPtr());

    // Clear post-process render target
    float clearColor[4] = { 0.39f, 0.58f, 0.92f, 1.0f };
    renderer->getDeviceContext()->ClearRenderTargetView(postProcessRTV, clearColor);
    renderer->getDeviceContext()->ClearDepthStencilView(renderer->getDepthStencilViewPtr(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    camera->update();
    XMMATRIX worldMatrix;
    XMMATRIX viewMatrix = camera->getViewMatrix();
    XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

    // -- Draw scene (floor, teapot, cube, sphere) as before --
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

    // 2. Render Sobel post-process to backbuffer
    renderer->setBackBufferRenderTarget();
    renderer->resetViewport();

    XMFLOAT2 texelSize(1.0f / postProcessWidth, 1.0f / postProcessHeight);
    postProcessShader->setShaderParameters(
        renderer->getDeviceContext(),
        postProcessSRV,
        texelSize
    );
    fullscreenQuad->sendData(renderer->getDeviceContext());
    postProcessShader->render(renderer->getDeviceContext(), fullscreenQuad->getIndexCount());

    // 3. Draw UI overlays
    gui();
    renderer->endScene();
    renderer->getDeviceContext()->RSSetState(nullptr);
}

// ImGui UI
void App1::gui()
{
    // Unbind geometry, hull, or domain shaders for safety
    renderer->getDeviceContext()->GSSetShader(nullptr, nullptr, 0);
    renderer->getDeviceContext()->HSSetShader(nullptr, nullptr, 0);
    renderer->getDeviceContext()->DSSetShader(nullptr, nullptr, 0);

    ImGui::Text("FPS: %.2f", timer->getFPS());
    ImGui::Checkbox("Wireframe mode", &wireframeToggle);
    ImGui::SliderFloat("Plane Height Scale", &heightScale, 1.0f, 100.0f);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}