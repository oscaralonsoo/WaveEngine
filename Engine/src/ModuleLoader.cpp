#include "ModuleLoader.h"
#define NOMINMAX  
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/cimport.h>
#include <windows.h>
#include <algorithm>
#include <limits>
#include <filesystem>
#include "Application.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "MeshImporter.h"
#include "ModelImporter.h"
#include "LibraryManager.h"  
#include "ComponentMaterial.h"
#include "MetaFile.h"
#include "Log.h"
#include "TextureImporter.h"
#include "ResourceMesh.h"     
#include "ResourceModel.h"     
#include "ResourceTexture.h"   
#include "ResourceModel.h"   
#include "AssetsWindow.h"

ModuleLoader::ModuleLoader() : Module() {}
ModuleLoader::~ModuleLoader() {}

bool ModuleLoader::Awake()
{
    return true;
}

bool ModuleLoader::Start()
{
    namespace fs = std::filesystem;

    // Verificar que LibraryManager esté inicializado
    if (!LibraryManager::IsInitialized()) {
        LOG_CONSOLE("[FileSystem] ERROR: LibraryManager not initialized");
        return false;
    }

    // Load default scene
    std::string defaultScenePath = "../Scene/scene.json";
    if (fs::exists(defaultScenePath)) {
        LOG_CONSOLE("[FileSystem] Loading default scene: %s", defaultScenePath.c_str());
        if (Application::GetInstance().scene->LoadScene(defaultScenePath)) {
            LOG_CONSOLE("[FileSystem] Default scene loaded successfully");
            return true;
        }
        else {
            LOG_CONSOLE("[FileSystem] WARNING: Failed to load default scene, using fallback geometry");
        }
    }

    // Usar las rutas de LibraryManager
    fs::path assetsPath = LibraryManager::GetAssetsRoot();

    if (!fs::exists(assetsPath) || !fs::is_directory(assetsPath)) {
        LOG_CONSOLE("[FileSystem] WARNING: Assets folder not accessible");

        // Fallback geometry
        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);
        Application::GetInstance().scene->RebuildOctree();

        return true;
    }

    // Cargar street
    fs::path streetPath = assetsPath / "Street" / "street2.fbx";
    LoadFbx(streetPath.generic_string());

    // Crear cámara de escena
    Application& app = Application::GetInstance();
    GameObject* cameraGO = app.scene->CreateGameObject("Camera");

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

    return true;
}

bool ModuleLoader::Update()
{
    if (Application::GetInstance().input->HasDroppedFile())
    {
        // Check if Assets Window is open and hovered
        ModuleEditor* editor = Application::GetInstance().editor.get();
        AssetsWindow* assetsWindow = editor ? editor->GetAssetsWindow() : nullptr;

        bool assetsWindowWillHandle = false;

        if (assetsWindow && assetsWindow->IsOpen())
        {
            // Check if Assets window is hovered - if so, let it handle the file
            if (assetsWindow->IsHovered())
            {
                assetsWindowWillHandle = true;
                LOG_CONSOLE("[FileSystem] Assets Window will handle the dropped file");
            }
        }

        // Only process here if Assets Window won't handle it
        if (!assetsWindowWillHandle)
        {
            std::string filePath = Application::GetInstance().input->GetDroppedFilePath();
            DroppedFileType fileType = Application::GetInstance().input->GetDroppedFileType();
            Application::GetInstance().input->ClearDroppedFile();

            LOG_CONSOLE("[FileSystem] Handling dropped file: %s", filePath.c_str());

            if (fileType == DROPPED_FBX)
            {
                if (LoadFbx(filePath))
                    LOG_CONSOLE("Fbx %s loaded properly", filePath.c_str());
                else
                    LOG_CONSOLE("Faile loading fbx %s", filePath.c_str());
            }
            else if (fileType == DROPPED_TEXTURE)
            {
                std::vector<GameObject*> selectedObjects =
                    Application::GetInstance().selectionManager->GetSelectedObjects();

                if (!selectedObjects.empty())
                {
                    int successCount = 0;
                    for (GameObject* obj : selectedObjects)
                    {
                        if (LoadTextureToGameObject(obj, filePath))
                        {
                            successCount++;
                        }
                    }
                    LOG_CONSOLE("[FileSystem] Texture applied to %d objects", successCount);
                }
                else
                {
                    Application::GetInstance().renderer->LoadTexture(filePath);
                    LOG_CONSOLE("[FileSystem] Texture loaded: %s", filePath.c_str());
                }
            }
        }
    }

    return true;
}


bool ModuleLoader::CleanUp()
{
    aiDetachAllLogStreams();
    LOG_CONSOLE("FileSystem cleaned up");
    return true;
}

bool ModuleLoader::LoadFbx(const std::string& fbxPath)
{
    // Cargar el modelo
    bool modelLoaded = false;
    UID modelUID = MetaFileManager::GetUIDFromAsset(fbxPath);
    
    if (modelUID != 0) {
        
        ResourceModel* resource = (ResourceModel*) Application::GetInstance().resources.get()->RequestResource(modelUID);

        if (resource)
        {
            nlohmann::json modelHierarchy = resource->GetModelHierarchy();
            GameObject* root = Application::GetInstance().scene->GetRoot();
            LOG_CONSOLE(modelHierarchy.dump().c_str());

            for (const auto& jsonNode : modelHierarchy) {

                GameObject::Deserialize(jsonNode, root);
            }

            Application::GetInstance().scene->RebuildOctree();
            LOG_CONSOLE("[FileSystem] Model instanciado desde Library con éxito: %s", fbxPath.c_str());
            modelLoaded = true;
        }

        Application::GetInstance().resources.get()->ReleaseResource(modelUID);
    }

    if (!modelLoaded)
    {
        LOG_CONSOLE("[FileSystem] Failed to load model, using fallback geometry.");

        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);
        Application::GetInstance().scene->RebuildOctree();
    }
        
    return modelLoaded;
}

bool ModuleLoader::LoadTextureToGameObject(GameObject* obj, const std::string& texturePath)
{
    if (!obj || !obj->IsActive())
        return false;

    bool applied = false;

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));

    if (meshComp && meshComp->IsActive() && meshComp->HasMesh())
    {
        ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
            obj->GetComponent(ComponentType::MATERIAL));

        if (matComp == nullptr)
        {
            matComp = static_cast<ComponentMaterial*>(
                obj->CreateComponent(ComponentType::MATERIAL));
        }

        if (matComp->LoadTexture(texturePath))
        {
            applied = true;
        }
    }

    // Apply recursively to children
    for (GameObject* child : obj->GetChildren())
    {
        if (LoadTextureToGameObject(child, texturePath))
        {
            applied = true;
        }
    }

    return applied;
}