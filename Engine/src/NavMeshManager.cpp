#include "Application.h"
#include "ModuleScene.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "Transform.h"
#include "FileSystem.h"
#include "Log.h"
#include "NavMeshManager.h"

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

    NavMeshData newMesh;
    newMesh.heightfield = hf;
    newMesh.owner = obj;

    navMeshes.push_back(newMesh);


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

    // --- TU CÓDIGO ACTUAL DE LAS LÍNEAS VERDES ---
    for (auto& meshData : navMeshes)
    {
        rcHeightfield* hf = meshData.heightfield;
        if (!hf) continue;

        for (int y = 0; y < hf->height; ++y)
        {
            for (int x = 0; x < hf->width; ++x)
            {
                float fx = hf->bmin[0] + x * hf->cs;
                float fz = hf->bmin[2] + y * hf->cs;

                const rcSpan* span = hf->spans[x + y * hf->width];

                while (span)
                {
                    if (span->area == RC_WALKABLE_AREA)
                    {
                        float fy = hf->bmin[1] + (span->smax) * hf->ch;

                        glm::vec3 p1 = { fx, fy, fz };
                        glm::vec3 p2 = { fx + hf->cs, fy, fz + hf->cs };

                        glm::vec4 navColor = glm::vec4(0, 1, 0, 1);

                        Application::GetInstance().renderer->DrawLine(p1, glm::vec3(p2.x, p1.y, p1.z), navColor);
                        Application::GetInstance().renderer->DrawLine(glm::vec3(p2.x, p1.y, p1.z), glm::vec3(p2.x, p1.y, p2.z), navColor);
                        Application::GetInstance().renderer->DrawLine(glm::vec3(p2.x, p1.y, p2.z), glm::vec3(p1.x, p1.y, p2.z), navColor);
                        Application::GetInstance().renderer->DrawLine(glm::vec3(p1.x, p1.y, p2.z), p1, navColor);
                    }
                    span = span->next;
                }
            }
        }


        // ---------------------------------------------

        // --- NUEVO: CÓDIGO PARA DIBUJAR LA CAJA DEL OBJETO BAKEADO ---

        // -------------------------------------------------------------

        if (meshData.owner && meshData.owner->IsActive()) {
            ComponentMesh* meshComp = static_cast<ComponentMesh*>(meshData.owner->GetComponent(ComponentType::MESH));

            if (meshComp != nullptr && meshComp->HasMesh()) {
                glm::vec3 worldMin, worldMax;

                // Asumiendo que tu ComponentMesh tiene esta función. 
                // Cámbiala por la forma en la que obtengas la AABB en tu motor si se llama distinto.
                meshComp->GetWorldAABB(worldMin, worldMax);

                // Naranja para diferenciarlo del verde del suelo y de la selección normal
                glm::vec3 bakedHighlightColor = glm::vec3(1.0f, 0.5f, 0.0f);

                Application::GetInstance().renderer->DrawAABB(worldMin, worldMax, bakedHighlightColor);
            }
        }
    }

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