#include "ModuleScene.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "SceneWindow.h"
#include <float.h>
#include <functional>
#include "ComponentMesh.h"
#include "ComponentCamera.h"
#include <nlohmann/json.hpp>
#include <fstream>

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

    // Resetear el flag después del rebuild
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
}

bool ModuleScene::Update()
{
    if (root)
    {
        root->Update();
    }

    if (needsOctreeRebuild)
    {
        LOG_DEBUG("Octree rebuild requested");

        // Verificar si ModuleEditor existe y si el gizmo está siendo usado
        ModuleEditor* editor = Application::GetInstance().editor.get();
        if (editor && editor->GetSceneWindow())
        {
            bool gizmoActive = editor->GetSceneWindow()->IsGizmoBeingUsed();
            LOG_DEBUG("Gizmo active: %s", gizmoActive ? "YES" : "NO");

            if (!gizmoActive)
            {
                LOG_DEBUG("Rebuilding octree now");
                RebuildOctree();
            }
            else
            {
                LOG_DEBUG("Skipping rebuild - gizmo is active");
            }
            // Si el gizmo está siendo usado, mantenemos el flag activo
            // para que se reconstruya cuando se suelte
        }
        else
        {
            LOG_DEBUG("No editor/scene window, rebuilding normally");
            // Si no hay editor o SceneWindow, reconstruir normalmente
            RebuildOctree();
        }
    }

    return true;
}

bool ModuleScene::PostUpdate()
{
    if (root)
    {
        CleanupMarkedObjects(root);
    }

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
            // Scene Camera
            ComponentCamera* cam = static_cast<ComponentCamera*>(child->GetComponent(ComponentType::CAMERA));
            if (cam && cam == Application::GetInstance().camera->GetSceneCamera())
            {
                Application::GetInstance().camera->SetSceneCamera(nullptr);
            }

            parent->RemoveChild(child);
            delete child;
        }
        else
        {
            CleanupMarkedObjects(child);
        }
    }
}

bool ModuleScene::SaveScene(const std::string& filepath)
{
    LOG_CONSOLE("Saving scene to: %s", filepath.c_str());

    nlohmann::json document;

    document["version"] = 1;

    // Serialize gameobjects
    nlohmann::json gameObjectsArray = nlohmann::json::array();

    if (root) {
        for (GameObject* child : root->GetChildren()) {
            child->Serialize(gameObjectsArray);
        }
    }

    document["gameObjects"] = gameObjectsArray;

    // Write to file
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("ERROR: Failed to open file for writing: %s", filepath.c_str());
        return false;
    }

    file << document.dump(4);
    file.close();

    LOG_CONSOLE("Scene saved successfully");
    return true;
}

bool ModuleScene::LoadScene(const std::string& filepath)
{
    LOG_CONSOLE("Loading scene from: %s", filepath.c_str());

    // Read file
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("ERROR: Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json document;

    try {
        file >> document;
    } catch (const nlohmann::json::parse_error& e) {
        LOG_CONSOLE("ERROR: Failed to parse JSON file: %s", e.what());
        file.close();
        return false;
    }

    file.close();

    // Clear selection to avoid bugs
    Application::GetInstance().selectionManager->ClearSelection();

    // Clear current scene
    ClearScene();

    // Deserialize GameObjects
    if (document.contains("gameObjects") && document["gameObjects"].is_array()) {
        const nlohmann::json& gameObjectsArray = document["gameObjects"];

        for (size_t i = 0; i < gameObjectsArray.size(); ++i) {
            GameObject* obj = GameObject::Deserialize(gameObjectsArray[i], root);
            if (!obj) {
                LOG_CONSOLE("WARNING: Failed to deserialize GameObject at index %zu", i);
            }
        }
    }

    // Relink Scene Camera
    if (root) {
        ComponentCamera* foundCamera = FindCameraInHierarchy(root);
        if (foundCamera) {
            Application::GetInstance().camera->SetSceneCamera(foundCamera);
        }
    }

    needsOctreeRebuild = true;

    LOG_CONSOLE("Scene loaded successfully");
    return true;
}

void ModuleScene::ClearScene()
{
    LOG_CONSOLE("Clearing scene...");

    if (!root) return;

    // Selection
    Application::GetInstance().selectionManager->ClearSelection();

    // Scene Camera
    Application::GetInstance().camera->SetSceneCamera(nullptr);

    // Childrens
    std::vector<GameObject*> children = root->GetChildren();
    for (GameObject* child : children) {
        root->RemoveChild(child);
        delete child;
    }

    // Octree
    if (octree) {
        octree->Clear();
    }

    LOG_CONSOLE("Scene cleared");
}

ComponentCamera* ModuleScene::FindCameraInHierarchy(GameObject* obj)
{
    if (!obj) return nullptr;

    ComponentCamera* cam = static_cast<ComponentCamera*>(obj->GetComponent(ComponentType::CAMERA));
    if (cam) return cam;

    for (GameObject* child : obj->GetChildren()) {
        ComponentCamera* foundCam = FindCameraInHierarchy(child);
        if (foundCam) return foundCam;
    }

    return nullptr;
}