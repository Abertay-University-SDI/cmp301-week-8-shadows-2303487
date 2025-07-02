// plane mesh
// Quad mesh made of many quads. Default is 100x100
#include "PlaneMesh.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Flat plane constructor
PlaneMesh::PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int lresolution)
{
    resolution = lresolution;
    useHeightmap = false;
    initBuffers(device);
}

// Heightmap-based constructor (const char*)
PlaneMesh::PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const char* heightmapPath, float lheightScale, int lresolution)
    : PlaneMesh(device, deviceContext, std::string(heightmapPath), lheightScale, lresolution)
{
}

// Heightmap-based constructor (std::string)
PlaneMesh::PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& heightmapPath, float lheightScale, int lresolution)
{
    resolution = lresolution;
    useHeightmap = true;
    this->heightmapPath = heightmapPath;
    this->heightScale = lheightScale;
    if (!loadHeightmap(heightmapPath, heightData, heightmapWidth, heightmapHeight)) {
        useHeightmap = false;
    }
    initBuffers(device);
}

PlaneMesh::~PlaneMesh()
{
    BaseMesh::~BaseMesh();
}

bool PlaneMesh::loadHeightmap(const std::string& filename, std::vector<float>& outData, int& outWidth, int& outHeight)
{
    int n = 0;
    unsigned char* imageData = stbi_load(filename.c_str(), &outWidth, &outHeight, &n, 1);
    if (!imageData) return false;

    outData.resize(outWidth * outHeight);
    for (int i = 0; i < outWidth * outHeight; ++i)
        outData[i] = imageData[i] / 255.0f;

    stbi_image_free(imageData);
    return true;
}

float PlaneMesh::sampleHeight(float u, float v) const
{
    if (!useHeightmap || heightData.empty()) return 0.0f;
    int x = static_cast<int>(u * (heightmapWidth - 1));
    int y = static_cast<int>(v * (heightmapHeight - 1));
    x = (x < 0) ? 0 : ((x >= heightmapWidth) ? heightmapWidth - 1 : x);
    y = (y < 0) ? 0 : ((y >= heightmapHeight) ? heightmapHeight - 1 : y);
    return heightData[y * heightmapWidth + x] * heightScale;
}

void PlaneMesh::initBuffers(ID3D11Device* device)
{
    VertexType* vertices;
    unsigned long* indices;
    int index, i, j;
    float positionX, positionZ, u, v, increment;
    D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;

    vertexCount = (resolution - 1) * (resolution - 1) * 6;
    indexCount = vertexCount;
    vertices = new VertexType[vertexCount];
    indices = new unsigned long[indexCount];

    index = 0;
    u = 0;
    v = 0;
    increment = 1.0f / (resolution - 1);

    for (j = 0; j < (resolution - 1); j++)
    {
        for (i = 0; i < (resolution - 1); i++)
        {
            auto make_vertex = [&](int x, int z, float u, float v) -> VertexType {
                VertexType vert;
                vert.position = XMFLOAT3((float)x, 0.0f, (float)z);
                vert.texture = XMFLOAT2(u, v);
                vert.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                if (useHeightmap)
                    vert.position.y = sampleHeight(u, v);
                return vert;
                };

            vertices[index] = make_vertex(i, j, u, v);
            indices[index] = index; index++;
            vertices[index] = make_vertex(i + 1, j + 1, u + increment, v + increment);
            indices[index] = index; index++;
            vertices[index] = make_vertex(i, j + 1, u, v + increment);
            indices[index] = index; index++;

            vertices[index] = make_vertex(i, j, u, v);
            indices[index] = index; index++;
            vertices[index] = make_vertex(i + 1, j, u + increment, v);
            indices[index] = index; index++;
            vertices[index] = make_vertex(i + 1, j + 1, u + increment, v + increment);
            indices[index] = index; index++;

            u += increment;
        }
        u = 0;
        v += increment;
    }

    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;
    vertexData.pSysMem = vertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;
    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;
    indexData.pSysMem = indices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;
    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);

    delete[] vertices;
    delete[] indices;
}