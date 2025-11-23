#include "ModuleScene.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Application.h"
#include <float.h>
#include <functional>
#include "ComponentMesh.h"

ModuleScene::ModuleScene() : Module()
{
    name = "ModuleScene";
    root = nullptr;
    renderer = nullptr;
    filesystem = nullptr;
}

ModuleScene::~ModuleScene()
{
    if (root)
    {
        delete root;
        root = nullptr;
    }
}

bool ModuleScene::Awake()
{
    return true;
}

bool ModuleScene::Start()
{
    LOG_DEBUG("Initializing Scene");
    renderer->DrawScene();
    root = new GameObject("Root");
    LOG_CONSOLE("Scene ready");


    LOG_DEBUG("Scene initialized - octree will be built when objects are loaded");
    LOG_CONSOLE("Scene manager ready");

    return true;
}
void ModuleScene::RebuildOctree()
{
    LOG_DEBUG("====================================");
    LOG_DEBUG("REBUILDING OCTREE");
    LOG_DEBUG("====================================");

    glm::vec3 sceneMin(std::numeric_limits<float>::max());
    glm::vec3 sceneMax(std::numeric_limits<float>::lowest());

    bool hasObjects = false;

    // Función para calcular bounds de todos los objetos
    std::function<void(GameObject*)> calculateBounds = [&](GameObject* obj) {
        if (!obj || !obj->IsActive()) return;

        ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
        if (mesh && mesh->IsActive() && mesh->HasMesh())
        {
            glm::vec3 objMin, objMax;
            mesh->GetWorldAABB(objMin, objMax);

            sceneMin = glm::min(sceneMin, objMin);
            sceneMax = glm::max(sceneMax, objMax);
            hasObjects = true;

            LOG_DEBUG("   Object '%s' AABB: min(%.2f,%.2f,%.2f) max(%.2f,%.2f,%.2f)",
                obj->GetName().c_str(),
                objMin.x, objMin.y, objMin.z,
                objMax.x, objMax.y, objMax.z);
        }

        for (GameObject* child : obj->GetChildren())
        {
            calculateBounds(child);
        }
        };

    if (root)
    {
        calculateBounds(root);
    }

    // Si no hay objetos con mesh, usar bounds por defecto
    if (!hasObjects)
    {
        sceneMin = glm::vec3(-10.0f, -10.0f, -10.0f);
        sceneMax = glm::vec3(10.0f, 10.0f, 10.0f);
        LOG_DEBUG(" No objects with meshes found, using default bounds");
    }
    else
    {
        // Expandir un poco los límites para dar margen (30%)
        glm::vec3 size = sceneMax - sceneMin;
        glm::vec3 margin = size * 0.3f;
        sceneMin -= margin;
        sceneMax += margin;
    }

    LOG_DEBUG("Octree bounds: min(%.2f,%.2f,%.2f) max(%.2f,%.2f,%.2f)",
        sceneMin.x, sceneMin.y, sceneMin.z,
        sceneMax.x, sceneMax.y, sceneMax.z);

    if (!octree)
    {
        LOG_DEBUG("Creating new octree with calculated bounds...");
        octree = std::make_unique<Octree>(sceneMin, sceneMax, 4, 5);
    }
    else
    {
        LOG_DEBUG("Clearing and recreating octree with calculated bounds...");
        octree->Clear();
        octree->Create(sceneMin, sceneMax, 4, 5);
    }

    LOG_DEBUG("Starting object collection...");
    LOG_DEBUG("   Root exists: %s", root ? "YES" : "NO");

    if (root)
    {
        LOG_DEBUG("   Root children count: %d", (int)root->GetChildren().size());

        // List all root children
        for (GameObject* child : root->GetChildren())
        {
            if (child)
            {
                LOG_DEBUG("   - Child: '%s' (active: %s)",
                    child->GetName().c_str(),
                    child->IsActive() ? "YES" : "NO");
            }
        }
    }

    // Insert all game objects
    int insertedCount = 0;
    int attemptedCount = 0;
    int skippedNoMesh = 0;
    int skippedInactive = 0;

    std::function<void(GameObject*)> insertRecursive = [&](GameObject* obj) {
        if (!obj)
        {
            LOG_DEBUG("    NULL object encountered");
            return;
        }

        attemptedCount++;
        LOG_DEBUG("    Checking '%s' (active: %s)",
            obj->GetName().c_str(),
            obj->IsActive() ? "YES" : "NO");

        if (!obj->IsActive())
        {
            LOG_DEBUG("      Skipped: inactive");
            skippedInactive++;
            return;
        }

        ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));

        if (!mesh)
        {
            LOG_DEBUG("       No mesh component");
            skippedNoMesh++;
        }
        else if (!mesh->IsActive())
        {
            LOG_DEBUG("       Mesh component inactive");
            skippedInactive++;
        }
        else if (!mesh->HasMesh())
        {
            LOG_DEBUG("       Mesh component has no mesh data");
            skippedNoMesh++;
        }
        else
        {
            LOG_DEBUG("      Has valid mesh, attempting insert...");

            if (octree->Insert(obj))
            {
                insertedCount++;
                LOG_DEBUG("       Successfully inserted '%s'", obj->GetName().c_str());
            }
            else
            {
                LOG_DEBUG("       Insert FAILED for '%s'", obj->GetName().c_str());

                // Get AABB to see why it failed
                glm::vec3 min, max;
                mesh->GetWorldAABB(min, max);
                LOG_DEBUG("         AABB: min(%.2f,%.2f,%.2f) max(%.2f,%.2f,%.2f)",
                    min.x, min.y, min.z, max.x, max.y, max.z);
            }
        }

        for (GameObject* child : obj->GetChildren())
        {
            insertRecursive(child);
        }
        };

    if (root)
    {
        insertRecursive(root);
    }

    LOG_DEBUG("====================================");
    LOG_DEBUG("=== Octree Rebuild Complete ===");
    LOG_DEBUG("Objects checked: %d", attemptedCount);
    LOG_DEBUG("Skipped (no mesh): %d", skippedNoMesh);
    LOG_DEBUG("Skipped (inactive): %d", skippedInactive);
    LOG_DEBUG("Successfully inserted: %d", insertedCount);
    LOG_DEBUG("Total nodes: %d", octree->GetTotalNodeCount());
    LOG_DEBUG("Total objects in octree: %d", octree->GetTotalObjectCount());
    LOG_DEBUG("====================================");

    if (insertedCount != octree->GetTotalObjectCount())
    {
        LOG_DEBUG(" WARNING: Mismatch between inserted (%d) and reported (%d)!",
            insertedCount, octree->GetTotalObjectCount());
    }

    LOG_CONSOLE("Octree rebuilt: %d objects in %d nodes (bounds: %.1fx%.1fx%.1f)",
        octree->GetTotalObjectCount(),
        octree->GetTotalNodeCount(),
        sceneMax.x - sceneMin.x,
        sceneMax.y - sceneMin.y,
        sceneMax.z - sceneMin.z);
}
void ModuleScene::UpdateObjectInOctree(GameObject* obj)
{
    if (!octree || !obj)
        return;

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(
        obj->GetComponent(ComponentType::MESH));

    if (!meshComp || !meshComp->HasMesh())
        return;

    // Remove from old position and reinsert
    octree->Remove(obj);
    octree->Insert(obj);

    LOG_DEBUG("Updated object '%s' in octree", obj->GetName().c_str());
}

bool ModuleScene::Update()
{
    if (root)
    {
        root->Update();
        CleanupMarkedObjects(root);
    }

    if (needsOctreeRebuild)
    {
        RebuildOctree();
    }
    return true;
}

bool ModuleScene::PostUpdate()
{
    return true;
}

bool ModuleScene::CleanUp()
{
    LOG_DEBUG("Cleaning up Scene");

    if (root)
    {
        delete root;
        root = nullptr;
    }

    if (octree)
    {
        octree->Clear();
        octree.reset();
    }

    return true;
}

GameObject* ModuleScene::CreateGameObject(const std::string& name)
{
    GameObject* newObject = new GameObject(name);

    if (root)
    {
        root->AddChild(newObject);
    }
    needsOctreeRebuild = true;
    return newObject;
}

void ModuleScene::CleanupMarkedObjects(GameObject* parent)
{
    if (!parent) return;

    std::vector<GameObject*> children = parent->GetChildren();

    for (GameObject* child : children)
    {
        if (child->IsMarkedForDeletion())
        {
            parent->RemoveChild(child);
            delete child;
        }
        else
        {
            CleanupMarkedObjects(child);
        }
    }
}