#include "ModuleScene.h"
#include "Renderer.h"
#include "FileSystem.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "Transform.h"
#include <float.h>
#include <functional>
#include "ComponentMesh.h"
#include "ComponentCamera.h"
#include "ComponentParticleSystem.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm> 
#include <cctype>    


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
    CreateChimneySmoke();
    LOG_CONSOLE("Scene ready");

    return true;
}

void ModuleScene::RebuildOctree()
{
    LOG_DEBUG("[ModuleScene] Starting full octree rebuild");

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
        // Expand bounds significantly (50% margin)
        glm::vec3 size = sceneMax - sceneMin;
        glm::vec3 margin = size * 0.5f;
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

    // Reset flag after rebuild
    needsOctreeRebuild = false;

    LOG_DEBUG("[ModuleScene] Octree rebuilt with %d objects", insertedCount);
    LOG_CONSOLE("Octree rebuilt: %d objects", insertedCount);
}

bool ModuleScene::Update()
{
    // Update all GameObjects
    if (root)
    {
        root->Update();
    }

    // Full rebuild only if explicitly requested
    if (needsOctreeRebuild)
    {
        LOG_DEBUG("[ModuleScene] Full octree rebuild requested");
        RebuildOctree();
    }

    return true;
}

bool ModuleScene::PostUpdate()
{
    // Full rebuild only if explicitly requested
    if (needsOctreeRebuild)
    {
        LOG_DEBUG("[ModuleScene] Full octree rebuild requested");
        RebuildOctree();
    }

    // Cleanup marked objects
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

            // Mark octree for rebuild after deletion
            needsOctreeRebuild = true;
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

    // Force full rebuild after loading scene
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

void ModuleScene::CreateChimneySmoke() {
    LOG_CONSOLE("Creating chimney smoke emitters");

    // Chimney positions
    std::vector<glm::vec3> chimneyPositions = {
        glm::vec3(2.271f, 0.530f, 1.9456f),   
        glm::vec3(-1.704f, 0.360f, -2.232f),  
        glm::vec3(2.146f, 0.155f, -0.839f),   
        glm::vec3(-2.219f, 0.399f, -1.693f),  
    };

    for (size_t i = 0; i < chimneyPositions.size(); i++) {
        GameObject* smokeEmitter = CreateGameObject("Chimney_Smoke_" + std::to_string(i + 1));

        Transform* trans = static_cast<Transform*>(
            smokeEmitter->GetComponent(ComponentType::TRANSFORM));
        if (trans) {
            trans->SetPosition(chimneyPositions[i]);
        }

        ComponentParticleSystem* particleSys = static_cast<ComponentParticleSystem*>(
            smokeEmitter->CreateComponent(ComponentType::PARTICLE));

        if (particleSys) {
            ConfigureSmokeEffect(particleSys);
            LOG_CONSOLE("Created smoke at position (%.2f, %.2f, %.2f)",
                chimneyPositions[i].x, chimneyPositions[i].y, chimneyPositions[i].z);
        }
    }

    LOG_CONSOLE("Chimney smoke setup complete! %d smoke emitters created", (int)chimneyPositions.size());
}

void ModuleScene::ConfigureSmokeEffect(ComponentParticleSystem* ps) {
    if (!ps || !ps->emitter) return;

    // Global setting
    ps->emitter->maxParticles = 60;        
    ps->emitter->emissionRate = 10.0f;     
    ps->emitter->additiveBlending = false; 
    ps->emitter->active = true;            

    // Module configuration
    for (auto mod : ps->emitter->modules) {
        if (mod->type == ParticleModuleType::SPAWNER) {
            ModuleEmitterSpawn* spawn = static_cast<ModuleEmitterSpawn*>(mod);

            spawn->shape = EmitterShape::CONE;
            spawn->emissionRadius = 0.1f;      
            spawn->coneAngle = 1.0f;           

            // Life 
            spawn->lifetimeMin = 1.0f;          
            spawn->lifetimeMax = 2.2f;

            // Speed
            spawn->speedMin = 0.003f;
            spawn->speedMax = 0.01f;

            // Color and opacity
            spawn->colorStart = glm::vec4(0.45f, 0.45f, 0.48f, 0.5f);
            spawn->colorEnd = glm::vec4(0.7f, 0.7f, 0.72f, 0.0f);

            // Size
            spawn->sizeStart = 0.05f;
            spawn->sizeEnd = 0.2f;

            // Rotation speed
            spawn->rotationSpeedMin = -10.0f;
            spawn->rotationSpeedMax = 10.0f;
        }
        else if (mod->type == ParticleModuleType::MOVEMENT) {
            ModuleEmitterMovement* move = static_cast<ModuleEmitterMovement*>(mod);

            
            move->gravity = glm::vec3(0.03f, 0.2f, 0.04f);
           
        }
    }

    LOG_DEBUG("Smoke effect configured for chimney");
}