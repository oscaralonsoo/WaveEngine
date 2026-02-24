#pragma once

#include "Module.h" 
#include "Recast.h"
#include "DetourNavMesh.h"

#include "Transform.h"
#include "ComponentMesh.h"

#include <vector>
#include <glm/glm.hpp>

#include "DetourNavMeshQuery.h"

class ModuleNavMesh : public Module
{
public:
    ModuleNavMesh();
    virtual ~ModuleNavMesh();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    void Bake(GameObject* obj);
    void DrawDebug();


private:

    void RecollectGeometry(GameObject* obj, std::vector<float>& vertices);
    void ExtractVertices(ComponentMesh* mesh, std::vector<float>& vertices);

    rcConfig CreateDefaultConfig(const float* minBounds, const float* maxBounds);
    void CalculateAABB(const std::vector<float>& verts, float* minBounds, float* maxBounds);

    rcPolyMesh* m_polyMesh = nullptr;
 /*   dtNavMesh* m_navMesh = nullptr;
    dtNavMeshQuery* m_navQuery = nullptr;

    rcHeightfield* bakedGroundData = nullptr;

    GameObject* lastBakedObject = nullptr;*/

    struct NavMeshData
    {
        rcHeightfield* heightfield = nullptr;
        dtNavMesh* navMesh = nullptr;
        dtNavMeshQuery* navQuery = nullptr;
        GameObject* owner = nullptr;
    };

    std::vector<NavMeshData> navMeshes;
};