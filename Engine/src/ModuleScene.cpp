#include "ModuleScene.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Application.h"
#include <float.h>
#include <functional>

ModuleScene::ModuleScene() : Module()
{
    name = "ModuleScene";
    root = nullptr;
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

    octree = std::make_unique<Octree>(
        glm::vec3(-100.0f, -100.0f, -100.0f),
        glm::vec3(100.0f, 100.0f, 100.0f),
        4,  // max objects per node
        5   // max depth
    );

    LOG_DEBUG("Octree initialized with default bounds");
    LOG_CONSOLE("Spatial partitioning system ready");

    return true;
}

void ModuleScene::RebuildOctree()
{
    if (!octree)
    {
        LOG_DEBUG("ERROR: Octree not initialized");
        return;
    }

    if (!root)
    {
        LOG_DEBUG("WARNING: No root GameObject, octree will be empty");
        return;
    }

    octree->Clear();

    // Ccalculate bounds of all the scene
    glm::vec3 sceneMin(FLT_MAX);
    glm::vec3 sceneMax(-FLT_MAX);
    bool hasObjects = false;

    std::function<void(GameObject*)> calculateBounds = [&](GameObject* obj) {
        if (!obj || !obj->IsActive()) return;

        ComponentMesh* meshComp = static_cast<ComponentMesh*>(
            obj->GetComponent(ComponentType::MESH));

        if (meshComp && meshComp->IsActive() && meshComp->HasMesh())
        {
            glm::vec3 worldMin, worldMax;
            meshComp->GetWorldAABB(worldMin, worldMax);

            sceneMin = glm::min(sceneMin, worldMin);
            sceneMax = glm::max(sceneMax, worldMax);
            hasObjects = true;
        }

        for (GameObject* child : obj->GetChildren())
        {
            calculateBounds(child);
        }
        };

    calculateBounds(root);

    if (!hasObjects)
    {
        LOG_DEBUG("No objects with meshes found, using default octree bounds");
        LOG_CONSOLE("Octree empty - no objects to partition");
        return;
    }

    // Expandir bounds para evitar objetos en los bordes
    glm::vec3 expansion = (sceneMax - sceneMin) * 0.1f; // 10% expansion
    expansion = glm::max(expansion, glm::vec3(5.0f)); // Mínimo 5 unidades
    sceneMin -= expansion;
    sceneMax += expansion;

    // recreate octree with new calculated bounds
    octree->Create(sceneMin, sceneMax, 4, 5);

    LOG_DEBUG("Octree bounds: min(%.2f, %.2f, %.2f) max(%.2f, %.2f, %.2f)",
        sceneMin.x, sceneMin.y, sceneMin.z,
        sceneMax.x, sceneMax.y, sceneMax.z);

    // insert all the gameobjects with mesh in the octree
    std::function<void(GameObject*)> insertIntoOctree = [&](GameObject* obj) {
        if (!obj || !obj->IsActive()) return;

        ComponentMesh* meshComp = static_cast<ComponentMesh*>(
            obj->GetComponent(ComponentType::MESH));

        if (meshComp && meshComp->IsActive() && meshComp->HasMesh())
        {
            bool inserted = octree->Insert(obj);
            if (!inserted)
            {
                LOG_DEBUG("WARNING: Failed to insert '%s' into octree",
                    obj->GetName().c_str());
            }
        }

        for (GameObject* child : obj->GetChildren())
        {
            insertIntoOctree(child);
        }
        };

    insertIntoOctree(root);

    int totalObjects = octree->GetTotalObjectCount();
    int totalNodes = octree->GetTotalNodeCount();

    LOG_DEBUG("Octree rebuilt: %d objects in %d nodes", totalObjects, totalNodes);
    LOG_CONSOLE("Spatial partitioning updated: %d objects in %d nodes",
        totalObjects, totalNodes);

    needsOctreeRebuild = false;
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