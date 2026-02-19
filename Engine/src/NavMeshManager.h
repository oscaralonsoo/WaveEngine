#pragma once

#include "Module.h" 
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"

#include "Transform.h"
#include "ComponentMesh.h"

#include <vector>
#include <glm/glm.hpp>

class ModuleNavMesh : public Module
{
public:
    ModuleNavMesh();
    virtual ~ModuleNavMesh();

    bool Start() override;
    bool CleanUp() override;

    void Bake(GameObject* obj);
    void DrawDebug();


private:

    void RecollectGeometry(GameObject* obj, std::vector<float>& vertices);
    void ExtractVertices(ComponentMesh* mesh, std::vector<float>& vertices);

    rcConfig CreateDefaultConfig(const float* minBounds, const float* maxBounds);
    void CalculateAABB(const std::vector<float>& verts, float* minBounds, float* maxBounds);

    rcPolyMesh* m_polyMesh = nullptr;
    dtNavMesh* m_navMesh = nullptr;
    dtNavMeshQuery* m_navQuery = nullptr;

    rcHeightfield* bakedGroundData = nullptr;
};