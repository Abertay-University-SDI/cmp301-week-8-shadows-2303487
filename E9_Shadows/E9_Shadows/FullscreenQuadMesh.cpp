#include "FullscreenQuadMesh.h"

FullscreenQuadMesh::FullscreenQuadMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    initBuffers(device);
}

FullscreenQuadMesh::~FullscreenQuadMesh()
{
    // BaseMesh releases buffers
}

void FullscreenQuadMesh::initBuffers(ID3D11Device* device)
{
    VertexType vertices[4];
    unsigned long indices[6];

    vertices[0].position = XMFLOAT3(-1, -1, 0); vertices[0].texture = XMFLOAT2(0, 1);
    vertices[1].position = XMFLOAT3(-1, 1, 0); vertices[1].texture = XMFLOAT2(0, 0);
    vertices[2].position = XMFLOAT3(1, 1, 0); vertices[2].texture = XMFLOAT2(1, 0);
    vertices[3].position = XMFLOAT3(1, -1, 0); vertices[3].texture = XMFLOAT2(1, 1);

    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 0; indices[4] = 2; indices[5] = 3;

    vertexCount = 4;
    indexCount = 6;

    // Describe and create vertex buffer
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = vertices;

    device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

    // Describe and create index buffer
    D3D11_BUFFER_DESC indexBufferDesc = {};
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA indexData = {};
    indexData.pSysMem = indices;

    device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
}