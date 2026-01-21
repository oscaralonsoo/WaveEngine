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
#include <nlohmann/json.hpp>
#include <fstream>

#include "ComponentAudioSource.h"
#include "ComponentAudioListener.h"
#include "Wwise_IDs.h"
#include "ModuleAudio.h"
#include <cmath>
#include <SDL3/SDL.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

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

// Al final de ModuleScene::Start(), reemplaza la inicialización del RTPC:

bool ModuleScene::Start()
{
    LOG_DEBUG("Initializing Scene");
    renderer->DrawScene();
    root = new GameObject("Root");
    LOG_CONSOLE("Scene ready");

    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio) {
        LOG_CONSOLE("[ERROR] ModuleAudio::Get() returned NULL!");
        return true;
    }
    LOG_CONSOLE("[DEBUG] ModuleAudio is available");

    // ... (código del listener) ...

    listenerObject = new GameObject("AudioListener");
    Transform* listenerTransform = static_cast<Transform*>(
        listenerObject->GetComponent(ComponentType::TRANSFORM)
        );
    listenerTransform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    ComponentAudioListener* listener = static_cast<ComponentAudioListener*>(
        listenerObject->CreateComponent(ComponentType::AUDIO_LISTENER)
        );

    root->AddChild(listenerObject);
    listenerObject->SetActive(true);

    if (listener) {
        listener->Enable();
    }

    LOG_CONSOLE("AudioListener created at (0, 0, 0)");

    // Static Audio Object
    staticAudioObject = new GameObject("StaticSFX");
    Transform* staticTransform = static_cast<Transform*>(
        staticAudioObject->GetComponent(ComponentType::TRANSFORM)
        );
    staticTransform->SetPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    ComponentAudioSource* staticSource = static_cast<ComponentAudioSource*>(
        staticAudioObject->CreateComponent(ComponentType::AUDIO_SOURCE)
        );

    root->AddChild(staticAudioObject);
    staticAudioObject->SetActive(true);

    if (staticSource) {
        staticSource->SetEvent(AK::EVENTS::SFX_STATIC_PLAY);
        staticSource->Enable();

        AkGameObjectID staticId = (AkGameObjectID)(uintptr_t)staticAudioObject;
        audio->RegisterAudioGameObject(staticId, "StaticSFX");

        audio->SetGameObjectTransform(staticId,
            5.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f
        );
    }

    LOG_CONSOLE("Static Audio Object created at (5, 0, 0)");

    // Dynamic Audio Object
    dynamicAudioObject = new GameObject("DynamicSFX");
    Transform* dynamicTransform = static_cast<Transform*>(
        dynamicAudioObject->GetComponent(ComponentType::TRANSFORM)
        );
    dynamicTransform->SetPosition(glm::vec3(0.0f, 0.0f, 4.0f));

    ComponentAudioSource* dynamicSource = static_cast<ComponentAudioSource*>(
        dynamicAudioObject->CreateComponent(ComponentType::AUDIO_SOURCE)
        );

    root->AddChild(dynamicAudioObject);
    dynamicAudioObject->SetActive(true);

    if (dynamicSource) {
        dynamicSource->SetEvent(AK::EVENTS::SFX_DYNAMIC_PLAY);
        dynamicSource->Enable();

        AkGameObjectID dynamicId = (AkGameObjectID)(uintptr_t)dynamicAudioObject;
        audio->RegisterAudioGameObject(dynamicId, "DynamicSFX");

        audio->SetGameObjectTransform(dynamicId,
            0.0f, 0.0f, 4.0f,
            0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f
        );
    }

    LOG_CONSOLE("Dynamic Audio Object created at (0, 0, 4)");

    // CORREGIDO: Inicializar RTPC en todos los lugares necesarios
    AkGameObjectID staticId = staticAudioObject ? (AkGameObjectID)(uintptr_t)staticAudioObject : 0;
    AkGameObjectID dynamicId = dynamicAudioObject ? (AkGameObjectID)(uintptr_t)dynamicAudioObject : 0;

    // Establecer RTPC inicial a 0 (sin efecto de túnel)
    audio->SetRTPCByName("TunnelAmount", 0.0f, 0);  // Global
    if (staticId)  audio->SetRTPCByName("TunnelAmount", 0.0f, staticId);
    if (dynamicId) audio->SetRTPCByName("TunnelAmount", 0.0f, dynamicId);

    tunnelAmount = 0.0f;
    tunnelTarget = 0.0f;
    tunnelInside = false;
    tunnelForce = false;
    tunnelForceValue = 0.0f;

    LOG_CONSOLE("Audio 3D GameObjects initialized successfully!");
    LOG_CONSOLE("Press PLAY to start 3D audio");
    LOG_CONSOLE("Press 'M' to toggle 3D audio on/off (only when playing)");
    LOG_CONSOLE("Press 'T' to toggle Tunnel FORCE test");
    LOG_CONSOLE("Press '1' to set Tunnel to 0 (normal)");
    LOG_CONSOLE("Press '2' to set Tunnel to 100 (full effect)");

    return true;
}

void ModuleScene::RebuildOctree()
{
    LOG_DEBUG("[ModuleScene] Starting full octree rebuild");

    glm::vec3 sceneMin(std::numeric_limits<float>::max());
    glm::vec3 sceneMax(std::numeric_limits<float>::lowest());

    bool hasObjects = false;

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

    if (!hasObjects)
    {
        sceneMin = glm::vec3(-10.0f, -10.0f, -10.0f);
        sceneMax = glm::vec3(10.0f, 10.0f, 10.0f);
    }
    else
    {
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

    needsOctreeRebuild = false;

    LOG_DEBUG("[ModuleScene] Octree rebuilt with %d objects", insertedCount);
    LOG_CONSOLE("Octree rebuilt: %d objects", insertedCount);
}

// En ModuleScene::Update(), reemplaza la sección del tunnel effect con esto:

// En ModuleScene::Update(), reemplaza la sección del tunnel effect con esto:

bool ModuleScene::Update()
{
    if (root)
    {
        root->Update();
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);

    static bool tKeyPressed = false;
    static bool key1Pressed = false;
    static bool key2Pressed = false;

    if (keys[SDL_SCANCODE_T] && !tKeyPressed)
    {
        tKeyPressed = true;
        tunnelForce = !tunnelForce;
        LOG_CONSOLE("[AUDIO] Tunnel FORCE = %s", tunnelForce ? "ON" : "OFF");
    }
    if (!keys[SDL_SCANCODE_T]) tKeyPressed = false;

    if (keys[SDL_SCANCODE_1] && !key1Pressed)
    {
        key1Pressed = true;
        tunnelForceValue = 0.0f;
        LOG_CONSOLE("[AUDIO] Tunnel FORCE value = 0");
    }
    if (!keys[SDL_SCANCODE_1]) key1Pressed = false;

    if (keys[SDL_SCANCODE_2] && !key2Pressed)
    {
        key2Pressed = true;
        tunnelForceValue = 100.0f;
        LOG_CONSOLE("[AUDIO] Tunnel FORCE value = 100");
    }
    if (!keys[SDL_SCANCODE_2]) key2Pressed = false;

    // SECCIÓN CORREGIDA DEL TUNNEL EFFECT
    if (listenerObject)
    {
        Transform* lt = static_cast<Transform*>(listenerObject->GetComponent(ComponentType::TRANSFORM));
        if (lt)
        {
            glm::vec3 p = lt->GetPosition();

            glm::vec3 tunnelMin = tunnelCenter - tunnelHalfSize;
            glm::vec3 tunnelMax = tunnelCenter + tunnelHalfSize;

            bool inside =
                (p.x >= tunnelMin.x && p.x <= tunnelMax.x) &&
                (p.y >= tunnelMin.y && p.y <= tunnelMax.y) &&
                (p.z >= tunnelMin.z && p.z <= tunnelMax.z);

            if (inside != tunnelInside)
            {
                tunnelInside = inside;
                tunnelTarget = tunnelInside ? 100.0f : 0.0f;
                LOG_CONSOLE("[AUDIO] Tunnel zone: %s (target: %.2f)",
                    tunnelInside ? "ENTER" : "EXIT", tunnelTarget);
            }

            // Interpolación suave
            const float smooth = 0.10f;
            tunnelAmount = tunnelAmount + (tunnelTarget - tunnelAmount) * smooth;

            // Valor final
            float finalTunnel = tunnelForce ? tunnelForceValue : tunnelAmount;

            ModuleAudio* audioMod = ModuleAudio::Get();
            if (audioMod)
            {
                audioMod->SetRTPCByName("TunnelAmount", finalTunnel, 0);
                static float lastPrinted = -9999.0f;
                
                audioMod->SetRTPCByName("TunnelAmount", tunnelAmount, 0);

                // justo debajo:
                float v = audioMod->GetRTPCCachedByName("TunnelAmount");
               


                if (staticAudioObject)
                {
                    AkGameObjectID staticId = (AkGameObjectID)(uintptr_t)staticAudioObject;
                    audioMod->SetRTPCByName("TunnelAmount", finalTunnel, staticId);
                }

                if (dynamicAudioObject)
                {
                    AkGameObjectID dynamicId = (AkGameObjectID)(uintptr_t)dynamicAudioObject;
                    audioMod->SetRTPCByName("TunnelAmount", finalTunnel, dynamicId);
                }
                audioMod->SetRTPC(AK::GAME_PARAMETERS::TUNNELAMOUNT, finalTunnel, audioMod->GetMusicGameObjectId());

                // Log solo cuando hay cambios significativos
                static float lastLoggedValue = -1.0f;
                if (std::abs(finalTunnel - lastLoggedValue) > 5.0f)
                {
                    LOG_CONSOLE("[AUDIO] TunnelAmount RTPC set to: %.2f (force: %s)",
                        finalTunnel, tunnelForce ? "YES" : "NO");
                    lastLoggedValue = finalTunnel;
                }
            }
        }
    }

    // Resto del código de SFX toggle...
    if (sfxStarted)
    {
        static bool mKeyPressed = false;

        if (keys[SDL_SCANCODE_M] && !mKeyPressed)
        {
            mKeyPressed = true;
            sfxEnabled = !sfxEnabled;

            ModuleAudio* audioMod = ModuleAudio::Get();
            if (audioMod && staticAudioObject && dynamicAudioObject)
            {
                AkGameObjectID staticId = (AkGameObjectID)(uintptr_t)staticAudioObject;
                AkGameObjectID dynamicId = (AkGameObjectID)(uintptr_t)dynamicAudioObject;

                if (sfxEnabled)
                {
                    LOG_CONSOLE("[AUDIO] 3D SFX enabled");
                    staticPlayingID = audioMod->PostEvent(AK::EVENTS::SFX_STATIC_PLAY, staticId);
                    dynamicPlayingID = audioMod->PostEvent(AK::EVENTS::SFX_DYNAMIC_PLAY, dynamicId);
                }
                else
                {
                    LOG_CONSOLE("[AUDIO] 3D SFX disabled");
                    AK::SoundEngine::StopAll(staticId);
                    AK::SoundEngine::StopAll(dynamicId);
                }
            }
        }

        if (!keys[SDL_SCANCODE_M])
        {
            mKeyPressed = false;
        }
    }

    // Movimiento del objeto dinámico
    if (dynamicAudioObject != nullptr && dynamicAudioObject->IsActive())
    {
        static float t = 0.0f;
        t += 0.02f;

        Transform* trans = static_cast<Transform*>(dynamicAudioObject->GetComponent(ComponentType::TRANSFORM));
        if (trans != nullptr)
        {
            float x = std::sinf(t) * 6.0f;
            float z = 4.0f + std::cosf(t) * 2.0f;
            trans->SetPosition(glm::vec3(x, 0.0f, z));
        }
    }

    if (needsOctreeRebuild)
    {
        LOG_DEBUG("[ModuleScene] Full octree rebuild requested");
        RebuildOctree();
    }

    return true;
}


bool ModuleScene::PostUpdate()
{
    if (needsOctreeRebuild)
    {
        LOG_DEBUG("[ModuleScene] Full octree rebuild requested");
        RebuildOctree();
    }

    if (root)
    {
        CleanupMarkedObjects(root);
    }

    return true;
}

bool ModuleScene::CleanUp()
{
    LOG_DEBUG("Cleaning up Scene");

    listenerObject = nullptr;
    staticAudioObject = nullptr;
    dynamicAudioObject = nullptr;

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
            ComponentCamera* cam = static_cast<ComponentCamera*>(child->GetComponent(ComponentType::CAMERA));
            if (cam && cam == Application::GetInstance().camera->GetSceneCamera())
            {
                Application::GetInstance().camera->SetSceneCamera(nullptr);
            }

            parent->RemoveChild(child);
            delete child;

            needsOctreeRebuild = true;
        }
        else
        {
            CleanupMarkedObjects(child);
        }
    }
}

void ModuleScene::StartAudio3D()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio || !staticAudioObject || !dynamicAudioObject) return;

    if (sfxEnabled)
    {
        AkGameObjectID staticId = (AkGameObjectID)(uintptr_t)staticAudioObject;
        AkGameObjectID dynamicId = (AkGameObjectID)(uintptr_t)dynamicAudioObject;

        LOG_CONSOLE("[AUDIO] Starting 3D SFX");

        staticPlayingID = audio->PostEvent(AK::EVENTS::SFX_STATIC_PLAY, staticId);
        dynamicPlayingID = audio->PostEvent(AK::EVENTS::SFX_DYNAMIC_PLAY, dynamicId);

        sfxStarted = true;
    }
}

void ModuleScene::StopAudio3D()
{
    if (!staticAudioObject || !dynamicAudioObject) return;

    AkGameObjectID staticId = (AkGameObjectID)(uintptr_t)staticAudioObject;
    AkGameObjectID dynamicId = (AkGameObjectID)(uintptr_t)dynamicAudioObject;

    LOG_CONSOLE("[AUDIO] Stopping 3D SFX");

    AK::SoundEngine::StopAll(staticId);
    AK::SoundEngine::StopAll(dynamicId);

    staticPlayingID = 0;
    dynamicPlayingID = 0;
    sfxStarted = false;
}

bool ModuleScene::SaveScene(const std::string& filepath)
{
    LOG_CONSOLE("Saving scene to: %s", filepath.c_str());

    nlohmann::json document;

    document["version"] = 1;

    nlohmann::json gameObjectsArray = nlohmann::json::array();

    if (root) {
        for (GameObject* child : root->GetChildren()) {
            child->Serialize(gameObjectsArray);
        }
    }

    document["gameObjects"] = gameObjectsArray;

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

    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("ERROR: Failed to open file for reading: %s", filepath.c_str());
        return false;
    }

    nlohmann::json document;

    try {
        file >> document;
    }
    catch (const nlohmann::json::parse_error& e) {
        LOG_CONSOLE("ERROR: Failed to parse JSON file: %s", e.what());
        file.close();
        return false;
    }

    file.close();

    Application::GetInstance().selectionManager->ClearSelection();

    ClearScene();

    if (document.contains("gameObjects") && document["gameObjects"].is_array()) {
        const nlohmann::json& gameObjectsArray = document["gameObjects"];

        for (size_t i = 0; i < gameObjectsArray.size(); ++i) {
            GameObject* obj = GameObject::Deserialize(gameObjectsArray[i], root);
            if (!obj) {
                LOG_CONSOLE("WARNING: Failed to deserialize GameObject at index %zu", i);
            }
        }
    }

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

    Application::GetInstance().selectionManager->ClearSelection();

    Application::GetInstance().camera->SetSceneCamera(nullptr);

    listenerObject = nullptr;
    staticAudioObject = nullptr;
    dynamicAudioObject = nullptr;

    std::vector<GameObject*> children = root->GetChildren();
    for (GameObject* child : children) {
        root->RemoveChild(child);
        delete child;
    }

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
