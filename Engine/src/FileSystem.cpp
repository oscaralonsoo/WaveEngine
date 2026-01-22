#include "FileSystem.h"
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
#include "LibraryManager.h"  
#include "ComponentMaterial.h"
#include "MetaFile.h"
#include "Log.h"
#include "TextureImporter.h"
#include "ResourceMesh.h"     
#include "ResourceTexture.h"   
#include "AssetsWindow.h"

FileSystem::FileSystem() : Module() {}
FileSystem::~FileSystem() {}

bool FileSystem::Awake()
{
    return true;
}

bool FileSystem::Start()
{
    namespace fs = std::filesystem;

    // Verificar que LibraryManager esté inicializado
    if (!LibraryManager::IsInitialized()) {
        LOG_CONSOLE("[FileSystem] ERROR: LibraryManager not initialized");
        return false;
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

    // Buscar el modelo
    fs::path streetPath = assetsPath / "Street";
    fs::path housePath = streetPath / "street2.fbx";

    if (!fs::exists(housePath)) {
        LOG_CONSOLE("[FileSystem] WARNING: street2.fbx not found at: %s", housePath.string().c_str());
        LOG_CONSOLE("[FileSystem] Using fallback geometry.");

        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);
        Application::GetInstance().scene->RebuildOctree();

        return true;
    }

    // Cargar el modelo
    GameObject* houseModel = LoadFBXAsGameObject(housePath.string());

    if (houseModel != nullptr)
    {
        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(houseModel);
        Application::GetInstance().scene->RebuildOctree();
        LOG_CONSOLE("[FileSystem] Model loaded successfully: %s", housePath.filename().string().c_str());
    }
    else
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

bool FileSystem::Update()
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
                GameObject* loadedModel = LoadFBXAsGameObject(filePath);
                if (loadedModel != nullptr)
                {
                    GameObject* root = Application::GetInstance().scene->GetRoot();
                    root->AddChild(loadedModel);
                    Application::GetInstance().scene->RebuildOctree();
                    LOG_CONSOLE("[FileSystem] FBX added to scene: %s", filePath.c_str());
                }
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
                        if (ApplyTextureToGameObject(obj, filePath))
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


bool FileSystem::CleanUp()
{
    aiDetachAllLogStreams();
    LOG_CONSOLE("FileSystem cleaned up");
    return true;
}

GameObject* FileSystem::LoadFBXAsGameObject(const std::string& file_path)
{
    // 1. Load/Create ONLY this file's .meta
    std::string metaPath = file_path + ".meta";
    MetaFile meta;

    if (std::filesystem::exists(metaPath)) {
        meta = MetaFile::Load(metaPath);

        if (meta.NeedsReimport(file_path)) {
            LOG_CONSOLE("[FileSystem] File modified, reimporting: %s", file_path.c_str());
        }
    }
    else {
        meta = MetaFileManager::GetOrCreateMeta(file_path);
        LOG_CONSOLE("[FileSystem] Created new .meta for: %s", file_path.c_str());
    }

    // 2. Build import flags from .meta settings
    unsigned int importFlags = aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ValidateDataStructure;

    if (meta.importSettings.generateNormals) {
        importFlags |= aiProcess_GenNormals;
    }

    if (meta.importSettings.flipUVs) {
        importFlags |= aiProcess_FlipUVs;
    }

    if (meta.importSettings.optimizeMeshes) {
        importFlags |= aiProcess_OptimizeMeshes;
    }

    // 3. Import the FBX
    const aiScene* scene = aiImportFile(file_path.c_str(), importFlags);

    if (scene == nullptr)
    {
        LOG_CONSOLE("ERROR: Failed to load model - %s", aiGetErrorString());
        return nullptr;
    }

    if (!scene->HasMeshes())
    {
        LOG_CONSOLE("ERROR: No geometry found in model");
        aiReleaseImport(scene);
        return nullptr;
    }

    std::string directory = file_path.substr(0, file_path.find_last_of("/\\"));

    LOG_CONSOLE("Found %d meshes, %d materials", scene->mNumMeshes, scene->mNumMaterials);

    int meshCounter = 0;
    GameObject* rootObj = ProcessNode(scene->mRootNode, scene, directory, meta.uid, meshCounter);

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    glm::mat4 identity(1.0f);
    CalculateBoundingBox(rootObj, minBounds, maxBounds, identity);

    NormalizeModelScale(rootObj, 5.0f);

    Transform* rootTransform = static_cast<Transform*>(
        rootObj->GetComponent(ComponentType::TRANSFORM));

    if (rootTransform && meta.uid != 0) {
        // 4. Apply import scale
        if (meta.importSettings.importScale != 1.0f) {
            glm::vec3 currentScale = rootTransform->GetScale();
            glm::vec3 newScale = currentScale * meta.importSettings.importScale;
            rootTransform->SetScale(newScale);
            LOG_DEBUG("[FileSystem] Applied import scale: %.3f", meta.importSettings.importScale);
        }

        glm::quat axisRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion

        // Apply Up Axis conversion
        // upAxis: 0=Y-Up (default), 1=Z-Up
        if (meta.importSettings.upAxis == 1) {
            // Convert Y-Up to Z-Up: Rotate -90° around X axis
            glm::quat upRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            axisRotation = upRotation * axisRotation;
            LOG_DEBUG("[FileSystem] Applying Z-Up conversion (rotate -90° on X)");
        }

        // Apply Front Axis conversion
        // frontAxis: 0=Z-Forward (default), 1=Y-Forward, 2=X-Forward
        if (meta.importSettings.frontAxis == 1) {
            // Convert Z-Forward to Y-Forward: Rotate 90° around X axis
            glm::quat frontRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            axisRotation = frontRotation * axisRotation;
            LOG_DEBUG("[FileSystem] Applying Y-Forward conversion (rotate 90° on X)");
        }
        else if (meta.importSettings.frontAxis == 2) {
            // Convert Z-Forward to X-Forward: Rotate -90° around Y axis
            glm::quat frontRotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            axisRotation = frontRotation * axisRotation;
            LOG_DEBUG("[FileSystem] Applying X-Forward conversion (rotate -90° on Y)");
        }

        // Combine with current rotation
        glm::quat currentRotation = rootTransform->GetRotationQuat();
        glm::quat newRotation = axisRotation * currentRotation;
        rootTransform->SetRotationQuat(newRotation);

        LOG_CONSOLE("[FileSystem] Applied axis conversion - UpAxis: %d, FrontAxis: %d",
            meta.importSettings.upAxis, meta.importSettings.frontAxis);
    }

    // 6. Update .meta ONLY if something changed
    bool metaChanged = false;

    long long currentTimestamp = MetaFileManager::GetFileTimestamp(file_path);
    if (meta.lastModified != currentTimestamp) {
        meta.lastModified = currentTimestamp;
        metaChanged = true;
        LOG_CONSOLE("[FileSystem] Updated timestamp for: %s", file_path.c_str());
    }

    if (metaChanged) {
        meta.Save(metaPath);
        LOG_CONSOLE("[FileSystem] Saved .meta for: %s", file_path.c_str());
    }

    aiReleaseImport(scene);

    if (rootObj) {
        Application::GetInstance().scene->ApplyPhysicsToAll(rootObj);
    }

    LOG_CONSOLE("Model loaded successfully: %s", file_path.c_str());

    return rootObj;
}

GameObject* FileSystem::ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory, UID baseUID, int& meshCounter)
{
    std::string nodeName = node->mName.C_Str();
    if (nodeName.empty()) nodeName = "Unnamed";

    GameObject* gameObject = new GameObject(nodeName);

    Transform* transform = static_cast<Transform*>(gameObject->GetComponent(ComponentType::TRANSFORM));

    if (transform != nullptr)
    {
        aiVector3D position, scaling;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scaling, rotation, position);

        transform->SetPosition(glm::vec3(position.x, position.y, position.z));
        transform->SetScale(glm::vec3(scaling.x, scaling.y, scaling.z));
        transform->SetRotationQuat(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));
    }

    // Process meshes for this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        aiMesh* aiMesh = scene->mMeshes[meshIndex];

        UID meshUID = ProcessMesh(aiMesh, scene, baseUID, meshCounter);
        meshCounter++; // Next mesh

        if (meshUID != 0)
        {
            ComponentMesh* meshComponent = static_cast<ComponentMesh*>(
                gameObject->CreateComponent(ComponentType::MESH));

            if (meshComponent)
            {
                if (!meshComponent->LoadMeshByUID(meshUID))
                {
                    LOG_CONSOLE("ERROR: Failed to load mesh with UID: %llu", meshUID);
                }
            }
        }

        // PROCESSING MATERIALS WITH EMBEDDED PROPERTIES
        if (aiMesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];

            ComponentMaterial* matComponent = static_cast<ComponentMaterial*>(
                gameObject->GetComponent(ComponentType::MATERIAL));

            if (matComponent == nullptr)
            {
                matComponent = static_cast<ComponentMaterial*>(
                    gameObject->CreateComponent(ComponentType::MATERIAL));
            }

            // extract diffuse color
            aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuseColor))
            {
                matComponent->SetDiffuseColor(glm::vec4(
                    diffuseColor.r,
                    diffuseColor.g,
                    diffuseColor.b,
                    diffuseColor.a
                ));
                LOG_DEBUG("Material diffuse color: (%.2f, %.2f, %.2f, %.2f)",
                    diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
            }

            // extract specular color (no implementado)
            aiColor4D specularColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &specularColor))
            {
                matComponent->SetSpecularColor(glm::vec4(
                    specularColor.r,
                    specularColor.g,
                    specularColor.b,
                    specularColor.a
                ));
                LOG_DEBUG("Material specular color: (%.2f, %.2f, %.2f)",
                    specularColor.r, specularColor.g, specularColor.b);
            }

            // Extraer color ambiente(no implementado)
            aiColor4D ambientColor(0.2f, 0.2f, 0.2f, 1.0f);
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &ambientColor))
            {
                matComponent->SetAmbientColor(glm::vec4(
                    ambientColor.r,
                    ambientColor.g,
                    ambientColor.b,
                    ambientColor.a
                ));
            }

            // Extraer color emisivo(no implementado)
            aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &emissiveColor))
            {
                matComponent->SetEmissiveColor(glm::vec4(
                    emissiveColor.r,
                    emissiveColor.g,
                    emissiveColor.b,
                    emissiveColor.a
                ));
            }

            // Cargar texturas difusas si existen
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString texturePath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

                std::string textureFile = texturePath.C_Str();
                std::string fileName;

                size_t lastSlash = textureFile.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    fileName = textureFile.substr(lastSlash + 1);
                else
                    fileName = textureFile;

                std::vector<std::string> searchPaths = {
                    directory + "\\" + fileName,
                    directory + "\\Textures\\" + fileName,
                };

                bool loaded = false;
                for (const auto& path : searchPaths)
                {
                    if (std::filesystem::exists(path))
                    {
                        if (matComponent->LoadTexture(path))
                        {
                            loaded = true;
                            LOG_DEBUG("Loaded diffuse texture: %s", fileName.c_str());
                            break;
                        }
                    }
                }

                if (!loaded)
                {
                    LOG_DEBUG("Diffuse texture not found: %s", fileName.c_str());
                }
            }
        }
    }

    // Process child nodes recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        GameObject* child = ProcessNode(node->mChildren[i], scene, directory, baseUID, meshCounter);
        if (child != nullptr)
        {
            gameObject->AddChild(child);
        }
    }

    return gameObject;
}

UID FileSystem::ProcessMesh(aiMesh* aiMesh, const aiScene* scene, UID baseUID, int meshIndex)
{
    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        LOG_CONSOLE("ERROR: ModuleResources not available");
        return 0;
    }

    // BaseUID (from .meta) + mesh index
    UID meshUID = baseUID + meshIndex;

    // Check if this UID already exists
    const auto& allResources = resources->GetAllResources();
    auto it = allResources.find(meshUID);
    if (it != allResources.end() && it->second->GetType() == Resource::MESH) {
        // Mesh already registered, return its UID
        return meshUID;
    }

    // Import and save mesh
    Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

    if (mesh.vertices.empty() || mesh.indices.empty()) {
        LOG_CONSOLE("ERROR: Failed to import mesh");
        return 0;
    }

    // Save to Library using UID-based filename
    std::string meshFilename = std::to_string(meshUID) + ".mesh";
    if (!MeshImporter::SaveToCustomFormat(mesh, meshFilename)) {
        LOG_CONSOLE("ERROR: Failed to save mesh to Library");
        return 0;
    }

    // Register mesh in ModuleResources
    std::string libraryPath = LibraryManager::GetMeshPathFromUID(meshUID);

    Resource* newResource = resources->CreateNewResourceWithUID(
        libraryPath.c_str(),  // Use library path as "asset" path for generated meshes
        Resource::MESH,
        meshUID
    );

    if (!newResource) {
        LOG_CONSOLE("ERROR: Failed to create resource");
        return 0;
    }

    newResource->SetLibraryFile(libraryPath);

    return meshUID;
}

void FileSystem::NormalizeModelScale(GameObject* rootObject, float targetSize)
{
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    glm::mat4 identity(1.0f);
    CalculateBoundingBox(rootObject, minBounds, maxBounds, identity);

    glm::vec3 size = maxBounds - minBounds;
    float maxDimension = std::max({ size.x, size.y, size.z });

    if (maxDimension > 0.0f)
    {
        float scale = targetSize / maxDimension;
        Transform* t = static_cast<Transform*>(rootObject->GetComponent(ComponentType::TRANSFORM));
        if (t != nullptr)
        {
            glm::vec3 currentScale = t->GetScale();
            t->SetScale(currentScale * scale);
        }

        LOG_CONSOLE("Model scaled to fit viewport (scale: %.4f)", scale);
    }
}

void FileSystem::CalculateBoundingBox(GameObject* obj, glm::vec3& minBounds, glm::vec3& maxBounds, const glm::mat4& parentTransform)
{
    Transform* t = static_cast<Transform*>(obj->GetComponent(ComponentType::TRANSFORM));
    glm::mat4 localTransform(1.0f);

    if (t != nullptr)
    {
        glm::vec3 pos = t->GetPosition();
        glm::vec3 scale = t->GetScale();
        glm::quat rot = t->GetRotationQuat();

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 rotation = glm::mat4_cast(rot);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

        localTransform = translation * rotation * scaleMatrix;
    }

    glm::mat4 worldTransform = parentTransform * localTransform;

    ComponentMesh* meshComp = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));
    if (meshComp != nullptr)
    {
        const Mesh& mesh = meshComp->GetMesh();

        for (const auto& vertex : mesh.vertices)
        {
            glm::vec4 worldPos = worldTransform * glm::vec4(vertex.position, 1.0f);
            glm::vec3 pos3(worldPos.x, worldPos.y, worldPos.z);

            minBounds.x = std::min(minBounds.x, pos3.x);
            minBounds.y = std::min(minBounds.y, pos3.y);
            minBounds.z = std::min(minBounds.z, pos3.z);

            maxBounds.x = std::max(maxBounds.x, pos3.x);
            maxBounds.y = std::max(maxBounds.y, pos3.y);
            maxBounds.z = std::max(maxBounds.z, pos3.z);
        }
    }

    for (GameObject* child : obj->GetChildren())
    {
        CalculateBoundingBox(child, minBounds, maxBounds, worldTransform);
    }
}

bool FileSystem::ApplyTextureToGameObject(GameObject* obj, const std::string& texturePath)
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
        if (ApplyTextureToGameObject(child, texturePath))
        {
            applied = true;
        }
    }

    return applied;
}