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

    return true;
}

void ModuleScene::RebuildOctree()
{
    glm::vec3 sceneMin(std::numeric_limits<float>::max());
    glm::vec3 sceneMax(std::numeric_limits<float>::lowest());

    bool hasObjects = false;

    // Calculate bounds of all objects
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

    // If no objects with mesh, use default bounds
    if (!hasObjects)
    {
        sceneMin = glm::vec3(-10.0f, -10.0f, -10.0f);
        sceneMax = glm::vec3(10.0f, 10.0f, 10.0f);
    }
    else
    {
        // Expand bounds slightly (30%)
        glm::vec3 size = sceneMax - sceneMin;
        glm::vec3 margin = size * 0.3f;
        sceneMin -= margin;
        sceneMax += margin;
    }

    if (!octree)
    {
        octree = std::make_unique<Octree>(sceneMin, sceneMax, 4, 5);
    }
    else
    {
        octree->Clear();
        octree->Create(sceneMin, sceneMax, 4, 5);
    }

    // Insert all game objects
    int insertedCount = 0;

    std::function<void(GameObject*)> insertRecursive = [&](GameObject* obj) {
        if (!obj || !obj->IsActive()) return;

        ComponentMesh* mesh = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));

        if (mesh && mesh->IsActive() && mesh->HasMesh())
        {
            if (octree->Insert(obj))
            {
                insertedCount++;
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