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
#include "ComponentRigidBody.h"
#include "Primitives.h"         
#include <nlohmann/json.hpp>
#include <fstream>
#include "ComponentCollider.h"
#include "ComponentBoxCollider.h"
#include "ComponentSphereCollider.h"
#include "ComponentMaterial.h"
#include "Input.h"
#include "Window.h"       // Necesario para GetScale()
#include "ComponentCamera.h"

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
    if (renderer) renderer->DrawScene();

    root = new GameObject("Root");

    FirstScene();

    LOG_CONSOLE("Escena creada: Suelo y 3 objetos con escalas y texturas aplicadas.");
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

    UpdateGameCamera();

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

    if (root) {
        ApplyPhysicsToAll(root);
    }

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

// En ModuleScene.cpp
void ModuleScene::ApplyPhysicsToAll(GameObject* obj) {
    if (!obj) return;

    ComponentMesh* mesh = (ComponentMesh*)obj->GetComponent(ComponentType::MESH);
    if (mesh != nullptr && obj->GetComponent(ComponentType::RIGIDBODY) == nullptr) 
    {
        glm::vec3 minB, maxB;
        mesh->GetLocalAABB(minB, maxB);
        
        glm::vec3 size = maxB - minB;
        glm::vec3 centerOffset = (minB + maxB) * 0.5f;

        ComponentBoxCollider* col = (ComponentBoxCollider*)obj->CreateComponent(ComponentType::COLLIDER_BOX);
        if (col) {
            col->SetSize(size); 
        }

        ComponentRigidBody* rb = (ComponentRigidBody*)obj->CreateComponent(ComponentType::RIGIDBODY);
        if (rb) {
            // APLICAR EL OFFSET AL RIGIDBODY
            rb->SetCenterOffset(centerOffset);
            rb->SetMass(1.0f);
            rb->Start();
        }
    }

    for (GameObject* child : obj->GetChildren()) {
        ApplyPhysicsToAll(child);
    }
}

void ModuleScene::FirstScene()
{
    Application& app = Application::GetInstance();

    GameObject* cameraGO = app.scene->CreateGameObject("MainCamera");

    Transform* transform = static_cast<Transform*>(
        cameraGO->GetComponent(ComponentType::TRANSFORM)
        );
    if (transform)
    {
        transform->SetPosition(glm::vec3(0.0f, 1.5f, 10.0f));
    }

    ComponentCamera* sceneCamera = static_cast<ComponentCamera*>(
        cameraGO->CreateComponent(ComponentType::CAMERA)
        );

    if (sceneCamera)
    {
        app.camera->SetSceneCamera(sceneCamera);
    }

    ComponentSphereCollider* col = (ComponentSphereCollider*)cameraGO->CreateComponent(ComponentType::COLLIDER_SPHERE);
    col->SetRadius(0.5f);

    // 2. Añadimos el RigidBody
    ComponentRigidBody* rb = (ComponentRigidBody*)cameraGO->CreateComponent(ComponentType::RIGIDBODY);
    rb->SetMass(1.0f); // Masa > 0 para que sea dinámico y colisione

    if (rb->GetRigidBody()) {
        rb->GetRigidBody()->setGravity(btVector3(0, 0, 0)); // Cámara flotante
        rb->GetRigidBody()->setDamping(0.9f, 0.9f); // Para que no deslice infinitamente
        rb->Start();
    }

    GameObject* floor = Primitives::CreateCubeGameObject("Floor", 0.0f);
    if (floor) {
        Transform* trans = (Transform*)floor->GetComponent(ComponentType::TRANSFORM);
        if (trans) {
            trans->SetPosition(glm::vec3(0.0f, -0.5f, 0.0f));
            trans->SetScale(glm::vec3(40.0f, 0.5f, 40.0f));
        }
    }

    GameObject* house1 = Primitives::CreateCubeGameObject("House1", 1.0f);
    if (house1) {
        Transform* trans = (Transform*)house1->GetComponent(ComponentType::TRANSFORM);
        if (trans) {
            trans->SetPosition(glm::vec3(15.0f, 10.0f, -10.0f));
            trans->SetScale(glm::vec3(5.0f, 7.0f, 5.0f));
        }
    }

    GameObject* house2 = Primitives::CreateCubeGameObject("House2", 1.0f);
    if (house2) {
        Transform* trans = (Transform*)house2->GetComponent(ComponentType::TRANSFORM);
        if (trans) {
            trans->SetPosition(glm::vec3(-7.0f, 10.0f, 18.0f));
            trans->SetScale(glm::vec3(5.0f, 7.0f, 5.0f));
        }
    }

    GameObject* car = Primitives::CreateCubeGameObject("Car", 1.0f);
    if (car) {
        Transform* trans = (Transform*)car->GetComponent(ComponentType::TRANSFORM);
        if (trans) {
            trans->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
            trans->SetScale(glm::vec3(2.0f, 1.0f, 5.0f));
        }
    }
}

// Asegúrate de tener estos includes arriba en ModuleScene.cpp:
// #include "Input.h"
// #include "ModuleWindow.h"
// #include "ComponentCamera.h"
// #include "ComponentRigidBody.h"
// #include "Transform.h"

void ModuleScene::UpdateGameCamera()
{
    // --- 1. VALIDACIONES ---
    if (!Application::GetInstance().camera) return;
    ComponentCamera* gameCam = Application::GetInstance().camera->GetSceneCamera();

    if (!gameCam || !gameCam->owner) return;

    Input* input = Application::GetInstance().input.get();
    if (!input) return;

    // --- 2. PREPARACIÓN ---
    float dt = 0.016f; // O tu App->time->dt
    float speed = 10.0f * dt;
    if (input->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT) speed *= 2.0f;

    // Ratón
    float mouseXf, mouseYf;
    SDL_GetMouseState(&mouseXf, &mouseYf);
    int scale = Application::GetInstance().window.get()->GetScale();
    mouseXf /= scale;
    mouseYf /= scale;

    static float lastMouseX = mouseXf;
    static float lastMouseY = mouseYf;
    float motionXf = mouseXf - lastMouseX;
    float motionYf = mouseYf - lastMouseY;
    lastMouseX = mouseXf;
    lastMouseY = mouseYf;

    // --- 3. LÓGICA DE MOVIMIENTO ---

    // A) CLICK DERECHO (Rotación + Vuelo)
    if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT || input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
    {
        if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) gameCam->ResetMouseInput();

        // 1. Rotar Cámara (Visual)
        gameCam->HandleMouseInput(mouseXf, mouseYf);

        // 2. Mover Posición (WASD)
        GameObject* camObj = gameCam->owner;
        Transform* trans = (Transform*)camObj->GetComponent(ComponentType::TRANSFORM);
        ComponentRigidBody* rb = (ComponentRigidBody*)camObj->GetComponent(ComponentType::RIGIDBODY);

        if (trans) {
            // Vectores locales
            glm::mat4 globalMat = trans->GetGlobalMatrix();
            glm::vec3 right = glm::vec3(globalMat[0]); 
            glm::vec3 forward = -glm::vec3(globalMat[2]); 
            glm::vec3 up = glm::vec3(0, 1, 0);

            glm::vec3 pos = trans->GetPosition();

            if (input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) pos += forward * speed;
            if (input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) pos -= forward * speed;
            if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) pos += right * speed;
            if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) pos -= right * speed;
            if (input->GetKey(SDL_SCANCODE_E) == KEY_REPEAT) pos += up * speed;
            if (input->GetKey(SDL_SCANCODE_Q) == KEY_REPEAT) pos -= up * speed;

            // 1. Aplicamos la posición al Transform visual
            trans->SetPosition(pos);

            // 2. OBLIGATORIO: Sincronizar el RigidBody al nuevo sitio
            if (rb && rb->GetRigidBody()) 
            {
                rb->SyncToBullet(); // <--- ESTO MUEVE EL RIGIDBODY A DONDE ESTÁ EL TRANSFORM
                
                // Matamos la inercia para que pare en seco al soltar la tecla
                rb->GetRigidBody()->setLinearVelocity(btVector3(0,0,0));
                rb->GetRigidBody()->activate(); 
            }
        }
    }
    // B) Orbitar
    else if ((input->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_RALT) == KEY_REPEAT) &&
        (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT || input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN))
    {
        if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) gameCam->ResetOrbitInput();
        gameCam->HandleOrbitInput(mouseXf, mouseYf);
    }
    // C) Paneo
    else if (input->GetMouseButtonDown(SDL_BUTTON_MIDDLE) == KEY_REPEAT || input->GetMouseButtonDown(SDL_BUTTON_MIDDLE) == KEY_DOWN)
    {
        if (input->GetMouseButtonDown(SDL_BUTTON_MIDDLE) == KEY_DOWN) gameCam->ResetPanInput();
        gameCam->HandlePanInput(motionXf, motionYf);
    }
}