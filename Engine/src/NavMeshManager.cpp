#include "NavMeshManager.h"
#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "Transform.h"
#include "FileSystem.h"
#include "Log.h"
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "Recast/Include/RecastAlloc.h"

ModuleNavMesh::ModuleNavMesh() : Module() {
    name = "ModuleNavMesh";
}

ModuleNavMesh::~ModuleNavMesh() {
    CleanUp();
}

bool ModuleNavMesh::Start() {
    LOG_CONSOLE("NavMesh Manager Started");
    return true;
}

void ModuleNavMesh::RecollectGeometry(GameObject* obj, std::vector<float>& vertices) 
{
    if (obj == nullptr || !obj->IsActive()) return;

    ComponentMesh* mesh = (ComponentMesh*)obj->GetComponent(ComponentType::MESH);

    if (mesh != nullptr && mesh->HasMesh()) {
        ExtractVertices(mesh, vertices);
    }

    for (GameObject* child : obj->GetChildren()) {
        RecollectGeometry(child, vertices);
    }
}

void ModuleNavMesh::ExtractVertices(ComponentMesh* mesh, std::vector<float>& vertices) {

    if (mesh == nullptr || !mesh->HasMesh()) return;

    //Local mesh points
    const Mesh& meshData = mesh->GetMesh();

    //Know the actual position of the mesh
    GameObject* owner = mesh->owner;
    if (!owner) return;

    Transform* trans = (Transform*)owner->GetComponent(ComponentType::TRANSFORM);
    if (!trans) return;

    glm::mat4 globalMat = trans->GetGlobalMatrix();
    
    //Traverse each vertex of the mesh to determine its position
    for (const auto& vertex : meshData.vertices) {

        glm::vec4 worldPos = globalMat * glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);

        vertices.push_back(worldPos.x);
        vertices.push_back(worldPos.y);
        vertices.push_back(worldPos.z);
    }
}

void ModuleNavMesh::Bake(GameObject* obj)
{
    std::vector<float> allVertices;

    if (obj != nullptr) {
        RecollectGeometry(obj, allVertices);
    }

    if (allVertices.empty()) {
        LOG_CONSOLE("NavMesh Error: No geometry found to bake!");
        return;
    }
       
    // min, max AABB
    float bmin[3], bmax[3];
    CalculateAABB(allVertices, bmin, bmax);

    //Recast Configuration
    rcConfig cfg = CreateDefaultConfig(bmin, bmax);
    rcContext ctx;

    // Create Height Map
    rcHeightfield* hf = rcAllocHeightfield();
    if (!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
        LOG_CONSOLE("NavMesh Error: Could not create heightfield.");
        return;
    }

    // Convert triangles to ground
    int nVerts = allVertices.size() / 3;
    std::vector<int> tris(nVerts);
    for (int i = 0; i < nVerts; ++i) tris[i] = i;

    int nTris = nVerts / 3;
    std::vector<unsigned char> areas(nTris, RC_WALKABLE_AREA);

    rcRasterizeTriangles(&ctx, &allVertices[0], nVerts, &tris[0], &areas[0], nTris, *hf, cfg.walkableClimb);

    LOG_CONSOLE("NavMesh Bake exitoso: %s. Vertices finales: %d", obj->GetName().c_str(), nVerts);

}

void ModuleNavMesh::CalculateAABB(const std::vector<float>& verts, float* minBounds, float* maxBounds) {
    minBounds[0] = maxBounds[0] = verts[0];
    minBounds[1] = maxBounds[1] = verts[1];
    minBounds[2] = maxBounds[2] = verts[2];

    for (size_t i = 3; i < verts.size(); i += 3) {
        rcVmin(minBounds, &verts[i]);
        rcVmax(maxBounds, &verts[i]);
    }
}

rcConfig ModuleNavMesh::CreateDefaultConfig(const float* minBounds, const float* maxBounds) {
    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.cs = 0.3f; // Cell size
    cfg.ch = 0.2f; // Cell height
    cfg.walkableSlopeAngle = 45.0f;
    cfg.walkableHeight = 10;
    cfg.walkableClimb = 2;
    cfg.walkableRadius = 2;

    rcVcopy(cfg.bmin, minBounds);
    rcVcopy(cfg.bmax, maxBounds);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    return cfg;
}

void ModuleNavMesh::DrawDebug() {
   if (bakedGroundData == nullptr) return;

    for (int y = 0; y < bakedGroundData->height; ++y) {
        for (int x = 0; x < bakedGroundData->width; ++x) {

            float fx = bakedGroundData->bmin[0] + x * bakedGroundData->cs;
            float fz = bakedGroundData->bmin[2] + y * bakedGroundData->cs;

            const rcSpan* span = bakedGroundData->spans[x + y * bakedGroundData->width];
            while (span) {
                if (span->area == RC_WALKABLE_AREA) {
                    float fy = bakedGroundData->bmin[1] + (span->smax) * bakedGroundData->ch;

                    glm::vec3 p1 = { fx, fy, fz };
                    glm::vec3 p2 = { fx + bakedGroundData->cs, fy, fz + bakedGroundData->cs };

                }
                span = span->next;
            }
        }
    }
}

bool ModuleNavMesh::CleanUp() {

    // Es vital liberar la memoria de Recast/Detour para evitar memory leaks
    if (m_polyMesh) rcFreePolyMesh(m_polyMesh);
    if (m_navMesh) dtFreeNavMesh(m_navMesh);
    if (m_navQuery) dtFreeNavMeshQuery(m_navQuery);

    m_polyMesh = nullptr;
    m_navMesh = nullptr;
    m_navQuery = nullptr;

    return true;
}