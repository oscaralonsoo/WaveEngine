#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "Transform.h"
#include "FileSystem.h"
#include "Log.h"
#include "NavMeshManager.h"
#include "ComponentNavigation.h"

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

bool ModuleNavMesh::Update() {
    DrawDebug(); // Llamamos a la función de dibujo
    return true;
}

void ModuleNavMesh::RecollectGeometry(GameObject* obj, std::vector<float>& vertices, std::vector<int>& indices)
{
    if (obj == nullptr || !obj->IsActive()) return;

    ComponentMesh* mesh = (ComponentMesh*)obj->GetComponent(ComponentType::MESH);
    if (mesh != nullptr && mesh->HasMesh())
        ExtractVertices(mesh, vertices, indices);

    for (GameObject* child : obj->GetChildren())
        RecollectGeometry(child, vertices, indices);
}

void ModuleNavMesh::ExtractVertices(ComponentMesh* mesh, std::vector<float>& vertices, std::vector<int>& indices)
{
    if (mesh == nullptr || !mesh->HasMesh()) return;

    const Mesh& meshData = mesh->GetMesh();
    GameObject* owner = mesh->owner;
    if (!owner) return;

    Transform* trans = (Transform*)owner->GetComponent(ComponentType::TRANSFORM);
    if (!trans) return;

    glm::mat4 globalMat = trans->GetGlobalMatrix();

    // Offset para que los índices apunten correctamente al buffer global
    int vertexOffset = vertices.size() / 3;

    for (const auto& vertex : meshData.vertices)
    {
        glm::vec4 worldPos = globalMat * glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
        vertices.push_back(worldPos.x);
        vertices.push_back(worldPos.y);
        vertices.push_back(worldPos.z);
    }

    // Usar los índices reales del mesh
    for (unsigned int idx : meshData.indices)
        indices.push_back(vertexOffset + (int)idx);
}

void ModuleNavMesh::Bake(GameObject* obj)
{
    RemoveNavMeshRecursive(obj);

    navObstacles.clear();

    RecollectObstacles(obj);

    std::vector<float> allVertices;
    std::vector<int>   allIndices;

    if (obj != nullptr)
        RecollectGeometry(obj, allVertices, allIndices);

    if (allVertices.empty() || allIndices.empty()) {
        LOG_CONSOLE("NavMesh Error: No geometry found to bake!");
        return;
    }

    ComponentNavigation* navComp = static_cast<ComponentNavigation*>(obj->GetComponent(ComponentType::NAVIGATION));

    if (navComp && navComp->type == NavType::OBSTACLE) {
        //navObstacles.push_back(obj);
        NavMeshData obstacleData;
        obstacleData.heightfield = nullptr;
        obstacleData.owner = obj;
        navMeshes.push_back(obstacleData);
        LOG_CONSOLE("Obstacle registered: %s", obj->GetName().c_str());
        return;
    }

    float bmin[3], bmax[3];
    CalculateAABB(allVertices, bmin, bmax);

    rcConfig cfg = CreateDefaultConfig(bmin, bmax);
    cfg.walkableSlopeAngle = navComp->maxSlopeAngle;

    rcContext ctx;

    rcHeightfield* hf = rcAllocHeightfield();
    if (!rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
        LOG_CONSOLE("NavMesh Error: Could not create heightfield.");
        return;
    }

    int nVerts = allVertices.size() / 3;
    int nTris = allIndices.size() / 3;

    std::vector<unsigned char> areas(nTris, RC_WALKABLE_AREA);

    rcRasterizeTriangles(&ctx, allVertices.data(), nVerts,
        allIndices.data(), areas.data(), nTris,
        *hf, cfg.walkableClimb);

    NavMeshData newMesh;
    newMesh.heightfield = hf;
    newMesh.owner = obj;
    navMeshes.push_back(newMesh);

    LOG_CONSOLE("NavMesh Bake exitoso: %s. Vertices: %d Triangulos: %d", obj->GetName().c_str(), nVerts, nTris);
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

    for (auto& meshData : navMeshes)
    {
        rcHeightfield* hf = meshData.heightfield;
        if (!hf) continue;

        for (int y = 0; y < hf->height; ++y)
        {
            for (int x = 0; x < hf->width; ++x)
            {
                auto GetCornerHeight = [&](int cx, int cy, unsigned short refSmax) -> float
                    {
                        // Las 4 celdas que comparten esta esquina
                        int offsets[4][2] = { {cx - 1, cy - 1}, {cx, cy - 1}, {cx - 1, cy}, {cx, cy} };

                        float totalHeight = 0.0f;
                        int   count = 0;

                        for (auto& off : offsets)
                        {
                            int nx = glm::clamp(off[0], 0, hf->width - 1);
                            int ny = glm::clamp(off[1], 0, hf->height - 1);

                            const rcSpan* s = hf->spans[nx + ny * hf->width];
                            const rcSpan* best = nullptr;
                            int bestDiff = INT_MAX;

                            while (s) {
                                int diff = abs((int)s->smax - (int)refSmax);
                                if (diff < bestDiff) { bestDiff = diff; best = s; }
                                s = s->next;
                            }

                            if (best) {
                                totalHeight += hf->bmin[1] + best->smax * hf->ch;
                                ++count;
                            }
                        }

                        return count > 0 ? totalHeight / count
                            : hf->bmin[1] + refSmax * hf->ch;
                    };

                const rcSpan* span = hf->spans[x + y * hf->width];

                while (span)
                {
                    if (span->area == RC_WALKABLE_AREA)
                    {
                        float fx = hf->bmin[0] + x * hf->cs;
                        float fz = hf->bmin[2] + y * hf->cs;
                        float fy = hf->bmin[1] + span->smax * hf->ch;

                        glm::vec3 quadMin = { fx, fy, fz };
                        glm::vec3 quadMax = { fx + hf->cs, fy, fz + hf->cs };

                        if (!IsBlockedByObstacle(quadMin, quadMax))
                        {
                            glm::vec3 p1 = { fx,          GetCornerHeight(x,     y,     span->smax), fz };
                            glm::vec3 p2 = { fx + hf->cs, GetCornerHeight(x + 1, y,     span->smax), fz };
                            glm::vec3 p3 = { fx + hf->cs, GetCornerHeight(x + 1, y + 1, span->smax), fz + hf->cs };
                            glm::vec3 p4 = { fx,          GetCornerHeight(x,     y + 1, span->smax), fz + hf->cs };

                            // ── Calcular el ángulo máximo de la celda ────────────────────────────
                            // Comprobamos las dos diagonales y los 4 bordes del quad
                            float diag = hf->cs * glm::sqrt(2.0f);
                            float maxDeltaH = 0.0f;
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p1.y - p2.y)); // borde N
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p2.y - p3.y)); // borde E
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p3.y - p4.y)); // borde S
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p4.y - p1.y)); // borde W
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p1.y - p3.y)); // diagonal
                            maxDeltaH = glm::max(maxDeltaH, glm::abs(p2.y - p4.y)); // diagonal

                            // atan2 con la horizontal más corta (borde) para ser conservadores
                            float cellAngle = glm::degrees(glm::atan(maxDeltaH, hf->cs));

                            ComponentNavigation* nav = (ComponentNavigation*)meshData.owner->GetComponent(ComponentType::NAVIGATION);
                            float slopeLimit = nav ? nav->maxSlopeAngle : 35.0f; 
                            
                            if (cellAngle > slopeLimit)
                            {
                                // Aplanar: todos los vértices a la altura central de la celda
                                float flatY = hf->bmin[1] + span->smax * hf->ch;
                                p1.y = p2.y = p3.y = p4.y = flatY;
                            }

                            glm::vec4 navColor = { 0, 1, 0, 1 };
                            Application::GetInstance().renderer->DrawLine(p1, p2, navColor);
                            Application::GetInstance().renderer->DrawLine(p2, p3, navColor);
                            Application::GetInstance().renderer->DrawLine(p3, p4, navColor);
                            Application::GetInstance().renderer->DrawLine(p4, p1, navColor);
                        }
                    }
                    span = span->next;
                }
            }
        }

        if (meshData.owner && meshData.owner->IsActive()) {
            ComponentMesh* meshComp = static_cast<ComponentMesh*>(meshData.owner->GetComponent(ComponentType::MESH));

            if (meshComp != nullptr && meshComp->HasMesh()) {
                glm::vec3 worldMin, worldMax;
                meshComp->GetWorldAABB(worldMin, worldMax);

                glm::vec3 bakedHighlightColor = glm::vec3(1.0f, 0.5f, 0.0f);
                Application::GetInstance().renderer->DrawAABB(worldMin, worldMax, bakedHighlightColor);
            }
        }
    }
}

void ModuleNavMesh::RecollectObstacles(GameObject* obj)
{
    if (!obj || !obj->IsActive()) return;

    ComponentNavigation* nav =
        (ComponentNavigation*)obj->GetComponent(ComponentType::NAVIGATION);

    if (nav && nav->type == NavType::OBSTACLE)
        navObstacles.push_back(obj);

    for (auto* child : obj->GetChildren())
        RecollectObstacles(child);
}


bool ModuleNavMesh::IsBlockedByObstacle(const glm::vec3& min, const glm::vec3& max)
{
    for (auto* obs : navObstacles)
    {
        if (!obs->IsActive()) continue;

        ComponentMesh* meshComp = static_cast<ComponentMesh*>(obs->GetComponent(ComponentType::MESH));
        if (!meshComp || !meshComp->HasMesh()) continue;

        glm::vec3 obsMin, obsMax;
        meshComp->GetWorldAABB(obsMin, obsMax);

        // AABB check
        if ((min.x <= obsMax.x && max.x >= obsMin.x) &&
            (min.y <= obsMax.y && max.y >= obsMin.y) &&
            (min.z <= obsMax.z && max.z >= obsMin.z))
        {
            return true;
        }
    }
    return false;
}

void ModuleNavMesh::RemoveNavMesh(GameObject* obj)
{
    if (!obj) return;

    for (auto it = navMeshes.begin(); it != navMeshes.end(); ++it)
    {
        if (it->owner == obj)
        {
            if (it->heightfield) rcFreeHeightField(it->heightfield);
            if (it->navMesh) dtFreeNavMesh(it->navMesh);
            if (it->navQuery) dtFreeNavMeshQuery(it->navQuery);

            navMeshes.erase(it);

            LOG_CONSOLE("NavMesh removed for object: %s", obj->GetName().c_str());
            return; 
        }
    }

    LOG_CONSOLE("NavMesh not found for object: %s", obj->GetName().c_str());
}

void ModuleNavMesh::RemoveNavMeshRecursive(GameObject* obj)
{
    if (!obj) return;

    for (auto& data : navMeshes)
    {
        if (data.owner == obj)
        {
            RemoveNavMesh(obj);
            break;
        }
    }

    for (auto* child : obj->GetChildren())
        RemoveNavMeshRecursive(child);
}



bool ModuleNavMesh::CleanUp() {

    // Es vital liberar la memoria de Recast/Detour para evitar memory leaks
    for (auto& mesh : navMeshes)
    {
        if (mesh.heightfield) rcFreeHeightField(mesh.heightfield);
        if (mesh.navMesh) dtFreeNavMesh(mesh.navMesh);
        if (mesh.navQuery) dtFreeNavMeshQuery(mesh.navQuery);
    }

    navMeshes.clear();
 
    

    return true;
}