/**
* \class Plane Mesh
*
* \brief Simple plane mesh object with optional heightmap-based displacement.
*
* Inherits from Base Mesh, Builds a simple plane with texture coordinates and normals.
* If a heightmap is provided, the plane's Y positions are displaced accordingly.
*
* \author Paul Robertson, extended by Copilot
*/

#ifndef _PLANEMESH_H_
#define _PLANEMESH_H_

#include "BaseMesh.h"
#include <string>
#include <vector>

class PlaneMesh : public BaseMesh
{
public:
    PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, int resolution = 100);
    PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const char* heightmapPath, float heightScale, int resolution = 100);
    PlaneMesh(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::string& heightmapPath, float heightScale, int resolution = 100);
    ~PlaneMesh();

protected:
    void initBuffers(ID3D11Device* device) override;

    bool useHeightmap = false;
    std::vector<float> heightData;
    int heightmapWidth = 0;
    int heightmapHeight = 0;
    float heightScale = 1.0f;
    std::string heightmapPath;

    int resolution;
    bool loadHeightmap(const std::string& filename, std::vector<float>& outData, int& outWidth, int& outHeight);
    float sampleHeight(float u, float v) const;
};

#endif