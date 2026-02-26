#pragma once

#include "Module.h" 
#include "Recast.h"
#include "DetourNavMesh.h"

#include "Transform.h"
#include "ComponentMesh.h"

#include <vector>
#include <glm/glm.hpp>

#include "DetourNavMeshQuery.h"

#include <algorithm>

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
    void RemoveNavMesh(GameObject* obj);
    void RemoveNavMeshRecursive(GameObject* obj);



private:

    void RecollectGeometry(GameObject* obj, std::vector<float>& vertices, std::vector<int>& indices);
    void ExtractVertices(ComponentMesh* mesh, std::vector<float>& vertices, std::vector<int>& indices);

    rcConfig CreateDefaultConfig(const float* minBounds, const float* maxBounds);
    void CalculateAABB(const std::vector<float>& verts, float* minBounds, float* maxBounds);


    bool IsBlockedByObstacle(const glm::vec3& min, const glm::vec3& max);
    void RecollectObstacles(GameObject* obj);

    rcPolyMesh* m_polyMesh = nullptr;

    struct NavMeshData
    {
        rcHeightfield* heightfield = nullptr;
        dtNavMesh* navMesh = nullptr;
        dtNavMeshQuery* navQuery = nullptr;
        GameObject* owner = nullptr;
    };

    std::vector<NavMeshData> navMeshes;
    std::vector<GameObject*> navObstacles;
};