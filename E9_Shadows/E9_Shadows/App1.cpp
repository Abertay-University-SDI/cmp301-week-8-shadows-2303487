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
    secondDirectionalLight = nullptr;      // <-- ADDED
    secondShadowMap = nullptr;             // <-- ADDED
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

    spotLightX = 50.0f;
    spotLightY = 30.0f;
    spotLightZ = 10.0f;
    spotFov = 70.0f;
    spotNear = 1.0f;
    spotFar = 100.0f;
    spotCutoffDegrees = 45.0f;
    spotExponent = 20.0f;
    shadowDebugMode = false;
    sobelToggle = false;
    heightScale = 25.0f;
    terrainResolution = 512;
    prevHeightScale = heightScale;
    prevTerrainResolution = terrainResolution;
}

App1::~App1()
{
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
    delete secondDirectionalLight; // <-- ADDED
    delete secondShadowMap;        // <-- ADDED

    if (shadowRasterState) { shadowRasterState->Release(); shadowRasterState = nullptr; }
    if (fullscreenQuad) { delete fullscreenQuad; fullscreenQuad = nullptr; }
    if (postProcessShader) { delete postProcessShader; postProcessShader = nullptr; }
    if (postProcessTexture) { postProcessTexture->Release(); postProcessTexture = nullptr; }
    if (postProcessRTV) { postProcessRTV->Release(); postProcessRTV = nullptr; }
    if (postProcessSRV) { postProcessSRV->Release(); postProcessSRV = nullptr; }
}

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

    postProcessWidth = width;
    postProcessHeight = height;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN)
{
    BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

    mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, terrainResolution);
    model = new AModel(renderer->getDevice(), "res/teapot.obj");
    textureMgr->loadTexture(L"brick", L"res/brick1.dds");
    cubeMesh = new CubeMesh(renderer->getDevice(), renderer->getDeviceContext());
    sphereMesh = new SphereMesh(renderer->getDevice(), renderer->getDeviceContext());

    textureShader = new TextureShader(renderer->getDevice(), hwnd);
    depthShader = new DepthShader(renderer->getDevice(), hwnd);
    shadowShader = new ShadowShader(renderer->getDevice(), hwnd);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 100;
    rasterDesc.SlopeScaledDepthBias = 2.0f;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;

    HRESULT hr = renderer->getDevice()->CreateRasterizerState(&rasterDesc, &shadowRasterState);
    if (FAILED(hr)) throw std::runtime_error("Failed to create rasterizer state!");

    int shadowmapWidth = 1024, shadowmapHeight = 1024;
    shadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
    spotShadowMap = new ShadowMap(renderer->getDevice(), 1024, 1024);

    fullscreenQuad = new FullscreenQuadMesh(renderer->getDevice(), renderer->getDeviceContext());
    postProcessShader = new PostProcessShader(renderer->getDevice(), hwnd);
    createPostProcessRenderTarget(screenWidth, screenHeight);

    int sceneWidth = 100, sceneHeight = 100;
    light = new Light();
    light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
    light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
    light->setDirection(-0.4f, -1.0f, 0.4f);
    light->setPosition(0.f, 30.f, -40.f);
    light->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

    spotLight = new Light();
    spotLight->setAmbientColour(0.2f, 0.2f, 0.2f, 1.0f);
    spotLight->setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f);

    updateSpotLight();

    spotLightProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(spotFov), 1.0f, spotNear, spotFar);
    spotShadowBias = 0.0005f;

    // Second Directional Light
    secondDirectionalLight = new Light();
    secondDirectionalLight->setAmbientColour(0.1f, 0.1f, 0.2f, 1.0f);
    secondDirectionalLight->setDiffuseColour(0.7f, 0.7f, 1.0f, 1.0f);
    secondDirectionalLight->setDirection(0.6f, -1.0f, -0.2f);
    secondDirectionalLight->setPosition(0.f, 40.f, 40.f);
    secondDirectionalLight->generateOrthoMatrix((float)sceneWidth, (float)sceneHeight, 0.1f, 100.f);

    secondShadowMap = new ShadowMap(renderer->getDevice(), shadowmapWidth, shadowmapHeight);
}

void App1::updateSpotLight()
{
    spotLight->setPosition(spotLightX, spotLightY, spotLightZ);

    XMFLOAT3 spotPos(spotLightX, spotLightY, spotLightZ);
    XMFLOAT3 spotTarget(0.0f, 0.0f, 0.0f);
    XMFLOAT3 spotDir(
        spotTarget.x - spotPos.x,
        spotTarget.y - spotPos.y,
        spotTarget.z - spotPos.z
    );
    float len = sqrtf(spotDir.x * spotDir.x + spotDir.y * spotDir.y + spotDir.z * spotDir.z);
    spotDir.x /= len; spotDir.y /= len; spotDir.z /= len;
    spotLight->setDirection(spotDir.x, spotDir.y, spotDir.z);

    if (spotFov < 90.0f) spotFov = 90.0f;
    if (spotFar < 150.0f) spotFar = 150.0f;
    if (spotLightY < 40.0f) spotLightY = 40.0f;

    spotLightProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(spotFov), 1.0f, spotNear, spotFar);
}

bool App1::frame()
{
    teapotAngle += 0.01f;
    if (teapotAngle > XM_2PI) teapotAngle -= XM_2PI;

    if (heightScale != prevHeightScale || terrainResolution != prevTerrainResolution) {
        delete mesh;
        mesh = new PlaneMesh(renderer->getDevice(), renderer->getDeviceContext(), "res/height.png", heightScale, terrainResolution);
        prevHeightScale = heightScale;
        prevTerrainResolution = terrainResolution;
    }

    static float prevSpotX = spotLightX, prevSpotY = spotLightY, prevSpotZ = spotLightZ;
    static float prevSpotFov = spotFov, prevSpotNear = spotNear, prevSpotFar = spotFar;
    if (prevSpotX != spotLightX || prevSpotY != spotLightY || prevSpotZ != spotLightZ ||
        prevSpotFov != spotFov || prevSpotNear != spotNear || prevSpotFar != spotFar)
    {
        updateSpotLight();
        prevSpotX = spotLightX;
        prevSpotY = spotLightY;
        prevSpotZ = spotLightZ;
        prevSpotFov = spotFov;
        prevSpotNear = spotNear;
        prevSpotFar = spotFar;
    }

    if (!BaseApplication::frame()) return false;
    if (!render()) return false;
    return true;
}

bool App1::render()
{
    depthPass();
    secondDirectionalDepthPass();
    spotDepthPass();
    finalPass();
    return true;
}

void App1::depthPass()
{
    renderer->getDeviceContext()->RSSetState(shadowRasterState);
    shadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
    light->generateViewMatrix();
    XMMATRIX lightViewMatrix = light->getViewMatrix();
    XMMATRIX lightProjectionMatrix = light->getOrthoMatrix();
    XMMATRIX worldMatrix;

    worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
    mesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

    XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
    XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
    XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
    model->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

    XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
    XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
    worldMatrix = cubeScale * cubeTranslate;
    cubeMesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

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

void App1::secondDirectionalDepthPass()
{
    renderer->getDeviceContext()->RSSetState(shadowRasterState);
    secondShadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
    secondDirectionalLight->generateViewMatrix();
    XMMATRIX lightViewMatrix = secondDirectionalLight->getViewMatrix();
    XMMATRIX lightProjectionMatrix = secondDirectionalLight->getOrthoMatrix();
    XMMATRIX worldMatrix;

    worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
    mesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

    XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
    XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
    XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
    model->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

    XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
    XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
    worldMatrix = cubeScale * cubeTranslate;
    cubeMesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, lightViewMatrix, lightProjectionMatrix);
    depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

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

void App1::spotDepthPass()
{
    renderer->getDeviceContext()->RSSetState(shadowRasterState);

    spotShadowMap->BindDsvAndSetNullRenderTarget(renderer->getDeviceContext());
    spotLight->generateViewMatrix();
    XMMATRIX spotViewMatrix = spotLight->getViewMatrix();
    XMMATRIX spotProjMatrix = spotLightProjMatrix;
    XMMATRIX worldMatrix;

    worldMatrix = XMMatrixTranslation(-50.f, 0.f, -10.f);
    mesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
    depthShader->render(renderer->getDeviceContext(), mesh->getIndexCount());

    XMMATRIX scaleMatrix = XMMatrixScaling(0.5f, 0.5f, 0.5f);
    XMMATRIX rotateMatrix = XMMatrixRotationY(teapotAngle);
    XMMATRIX translateMatrix = XMMatrixTranslation(0.f, 7.f, 5.f);
    worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
    model->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
    depthShader->render(renderer->getDeviceContext(), model->getIndexCount());

    XMMATRIX cubeScale = XMMatrixScaling(4.f, 4.f, 4.f);
    XMMATRIX cubeTranslate = XMMatrixTranslation(20.f, 2.f, 0.f);
    worldMatrix = cubeScale * cubeTranslate;
    cubeMesh->sendData(renderer->getDeviceContext());
    depthShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, spotViewMatrix, spotProjMatrix);
    depthShader->render(renderer->getDeviceContext(), cubeMesh->getIndexCount());

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

void App1::finalPass()
{
    renderer->getDeviceContext()->RSSetState(shadowRasterState);
    renderer->getDeviceContext()->OMSetRenderTargets(1, &postProcessRTV, renderer->getDepthStencilViewPtr());

    float clearColor[4] = { 0.39f, 0.58f, 0.92f, 1.0f };
    renderer->getDeviceContext()->ClearRenderTargetView(postProcessRTV, clearColor);
    renderer->getDeviceContext()->ClearDepthStencilView(renderer->getDepthStencilViewPtr(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

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
        secondShadowMap->getDepthMapSRV(),
        light,
        spotLight,
        secondDirectionalLight,
        cos(XMConvertToRadians(spotCutoffDegrees)),
        spotExponent,
        shadowDebugMode,
        spotShadowBias
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
        secondShadowMap->getDepthMapSRV(),
        light,
        spotLight,
        secondDirectionalLight,
        cos(XMConvertToRadians(spotCutoffDegrees)),
        spotExponent,
        shadowDebugMode,
        spotShadowBias
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
        secondShadowMap->getDepthMapSRV(),
        light,
        spotLight,
        secondDirectionalLight,
        cos(XMConvertToRadians(spotCutoffDegrees)),
        spotExponent,
        shadowDebugMode,
        spotShadowBias
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
        secondShadowMap->getDepthMapSRV(),
        light,
        spotLight,
        secondDirectionalLight,
        cos(XMConvertToRadians(spotCutoffDegrees)),
        spotExponent,
        shadowDebugMode,
        spotShadowBias
    );
    shadowShader->render(renderer->getDeviceContext(), sphereMesh->getIndexCount());

    renderer->setBackBufferRenderTarget();
    renderer->resetViewport();

    if (sobelToggle)
    {
        XMFLOAT2 texelSize(1.0f / postProcessWidth, 1.0f / postProcessHeight);
        postProcessShader->setShaderParameters(
            renderer->getDeviceContext(),
            postProcessSRV,
            texelSize
        );
        fullscreenQuad->sendData(renderer->getDeviceContext());
        postProcessShader->render(renderer->getDeviceContext(), fullscreenQuad->getIndexCount());
    }
    else
    {
        XMMATRIX identity = XMMatrixIdentity();
        textureShader->setShaderParameters(renderer->getDeviceContext(), identity, XMMatrixIdentity(), XMMatrixIdentity(), postProcessSRV);
        fullscreenQuad->sendData(renderer->getDeviceContext());
        textureShader->render(renderer->getDeviceContext(), fullscreenQuad->getIndexCount());
    }

    gui();
    renderer->endScene();
    renderer->getDeviceContext()->RSSetState(nullptr);
}

void App1::gui()
{
    renderer->getDeviceContext()->GSSetShader(nullptr, nullptr, 0);
    renderer->getDeviceContext()->HSSetShader(nullptr, nullptr, 0);
    renderer->getDeviceContext()->DSSetShader(nullptr, nullptr, 0);

    ImGui::Text("FPS: %.2f", timer->getFPS());
    ImGui::Checkbox("Wireframe mode", &wireframeToggle);
    ImGui::Checkbox("Shadow Debug Mode", &shadowDebugMode);
    ImGui::Checkbox("Sobel mode", &sobelToggle);
    ImGui::SliderFloat("Plane Height Scale", &heightScale, 1.0f, 100.0f);
    ImGui::SliderInt("Terrain Resolution", &terrainResolution, 100, 2048);

    ImGui::Separator();
    ImGui::Text("Spotlight Controls");
    ImGui::SliderFloat("Spot X", &spotLightX, -100.0f, 100.0f);
    ImGui::SliderFloat("Spot Y", &spotLightY, 0.0f, 100.0f);
    ImGui::SliderFloat("Spot Z", &spotLightZ, -100.0f, 100.0f);
    ImGui::SliderFloat("Spot FOV", &spotFov, 10.0f, 120.0f);
    ImGui::SliderFloat("Spot Near", &spotNear, 0.1f, 10.0f);
    ImGui::SliderFloat("Spot Far", &spotFar, 10.0f, 200.0f);

    ImGui::SliderFloat("Spot Shadow Bias", &spotShadowBias, 0.0001f, 0.005f, "%.5f");

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}