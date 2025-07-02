#pragma once
#include "DXF.h"

class FullscreenQuadMesh : public BaseMesh
{
public:
    FullscreenQuadMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    ~FullscreenQuadMesh();

protected:
    void initBuffers(ID3D11Device* device) override;
};