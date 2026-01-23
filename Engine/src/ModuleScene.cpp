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
#include "Window.h"    
#include "ComponentCamera.h"
#include "GameObject.h"
#include "ComponentP2PConstraint.h"

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

    ComponentCamera* sceneCamera = static_cast<ComponentCamera*>(
        cameraGO->CreateComponent(ComponentType::CAMERA)
        );

    if (sceneCamera)
    {
        app.camera->SetSceneCamera(sceneCamera);
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

    GameObject* anchor = Primitives::CreateCubeGameObject("Initial_Anchor", 0.0f);
    if (anchor) {
        Transform* tA = (Transform*)anchor->GetComponent(ComponentType::TRANSFORM);
        if (tA) tA->SetPosition(glm::vec3(0.0f, 15.0f, 0.0f));
        
        // Importante: si tienes una función para registrar la física en el editor, úsala
        // Si no, Primitives::CreateCubeGameObject ya debería haberlo añadido al mundo
    }

    // 2. Crear el objeto dinámico (Cubo que colgará)
    GameObject* dynamicObj = Primitives::CreateCubeGameObject("Initial_Dynamic", 1.0f);
    if (dynamicObj) {
        Transform* tB = (Transform*)dynamicObj->GetComponent(ComponentType::TRANSFORM);
        // Lo ponemos a 5 unidades de distancia (largo de la cuerda)
        if (tB) tB->SetPosition(glm::vec3(5.0f, 15.0f, 0.0f));

        // 3. Crear y configurar el Constraint P2P
        ComponentP2PConstraint* p2p = (ComponentP2PConstraint*)dynamicObj->CreateComponent(ComponentType::CONSTRAINT_P2P);
        
        if (p2p && anchor) {
            p2p->SetTarget(anchor);
            
            // Definimos los puntos de anclaje (Pivots)
            // Para una cuerda de 5m:
            p2p->SetPivots(glm::vec3(-2.5f, 0.0f, 0.0f), glm::vec3(2.5f, 0.0f, 0.0f));
            
            // Forzamos la creación en Bullet
            p2p->CreateConstraint();
        }
    }
}

void ModuleScene::UpdateGameCamera()
{
    // Usamos la instancia correcta de Application
    Application& app = Application::GetInstance();
    
    ComponentCamera* gameCam = app.camera->GetActiveCamera();
    
    // ERROR FIX: En tus archivos, Component hereda de owner, pero se accede 
    // mediante el miembro 'owner' (que es protected/public) no mediante GetOwner()
    if (gameCam == nullptr || gameCam->owner == nullptr) return;

    // Obtenemos los componentes desde el owner directamente
    Transform* trans = (Transform*)gameCam->owner->GetComponent(ComponentType::TRANSFORM);
    ComponentRigidBody* rb = (ComponentRigidBody*)gameCam->owner->GetComponent(ComponentType::RIGIDBODY);
    
    if (!trans) return;

    Input* input = app.input.get(); // Obtenemos el puntero del shared_ptr
    float mouseXf = input->GetMouseX(); // Usa GetMouseXf() para floats según tus archivos
    float mouseYf = input->GetMouseY();

    glm::vec3 pos = trans->GetPosition();
    glm::vec3 front = gameCam->GetFront();
    glm::vec3 up = gameCam->GetUp();
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    
    // ERROR FIX: App->GetDeltaTime() -> app.time->GetDeltaTime() (según tu Application.h)
    float speed = gameCam->GetMovementSpeed() * app.time->GetDeltaTime();

    bool hasMoved = false;

    // A) MOVIMIENTO LIBRE
    if (input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT)
    {
        if (input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) { pos += front * speed; hasMoved = true; }
        if (input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) { pos -= front * speed; hasMoved = true; }
        if (input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) { pos -= right * speed; hasMoved = true; }
        if (input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) { pos += right * speed; hasMoved = true; }
        if (input->GetKey(SDL_SCANCODE_E) == KEY_REPEAT) { pos += up * speed; hasMoved = true; }
        if (input->GetKey(SDL_SCANCODE_Q) == KEY_REPEAT) { pos -= up * speed; hasMoved = true; }

        if (hasMoved) {
            trans->SetPosition(pos);
        }

        gameCam->HandleMouseInput(mouseXf, mouseYf);
        hasMoved = true; 
    }
    // B) ORBITAR
    else if ((input->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_RALT) == KEY_REPEAT) &&
             (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT || input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN))
    {
        if (input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) gameCam->ResetOrbitInput();
        gameCam->HandleOrbitInput(mouseXf, mouseYf);
        hasMoved = true;
    }
    // C) PANEO
    else if (input->GetMouseButtonDown(SDL_BUTTON_MIDDLE) == KEY_REPEAT || 
            ((input->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT || input->GetKey(SDL_SCANCODE_RALT) == KEY_REPEAT) && 
              input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT))
    {
        if (input->GetMouseButtonDown(SDL_BUTTON_MIDDLE) == KEY_DOWN || input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) 
            gameCam->ResetPanInput();
        
        gameCam->HandlePanInput(mouseXf, mouseYf);
        hasMoved = true;
    }

    // --- SINCRONIZACIÓN FINAL ---
    if (hasMoved && rb && rb->GetRigidBody())
    {
        rb->SyncToBullet();
        
        // Limpiamos fuerzas para evitar que la física "empuje" a la cámara tras el movimiento manual
        rb->GetRigidBody()->setLinearVelocity(btVector3(0, 0, 0));
        rb->GetRigidBody()->setAngularVelocity(btVector3(0, 0, 0));
        rb->GetRigidBody()->activate(true);
    }
}