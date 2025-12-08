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

FileSystem::FileSystem() : Module() {}
FileSystem::~FileSystem() {}

bool FileSystem::Awake()
{
    return true;
}

bool FileSystem::Start()
{
    // Get executable directory
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string execPath(buffer);

    size_t pos = execPath.find_last_of("\\/");
    std::string currentDir = execPath.substr(0, pos);

    // Go up 2 levels from executable (build/Debug/ -> build/ -> ProjectRoot/)
    pos = currentDir.find_last_of("\\/");
    if (pos != std::string::npos) {
        currentDir = currentDir.substr(0, pos);
        pos = currentDir.find_last_of("\\/");
        if (pos != std::string::npos) {
            currentDir = currentDir.substr(0, pos);
        }
    }

    // Check if Assets folder exists
    std::string assetsPath = currentDir + "\\Assets\\Street";
    DWORD attribs = GetFileAttributesA(assetsPath.c_str());
    bool assetsFound = (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));

    if (!assetsFound)
    {
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

    std::string housePath = assetsPath + "\\street2.fbx";

    GameObject* houseModel = LoadFBXAsGameObject(housePath);

    if (houseModel != nullptr)
    {
        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(houseModel);
        Application::GetInstance().scene->RebuildOctree();
    }
    else
    {
        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);
        Application::GetInstance().scene->RebuildOctree();
    }

    return true;
}

bool FileSystem::Update()
{
    if (Application::GetInstance().input->HasDroppedFile())
    {
        std::string filePath = Application::GetInstance().input->GetDroppedFilePath();
        DroppedFileType fileType = Application::GetInstance().input->GetDroppedFileType();
        Application::GetInstance().input->ClearDroppedFile();

        if (fileType == DROPPED_FBX)
        {
            GameObject* loadedModel = LoadFBXAsGameObject(filePath);
            if (loadedModel != nullptr)
            {
                GameObject* root = Application::GetInstance().scene->GetRoot();
                root->AddChild(loadedModel);
                Application::GetInstance().scene->RebuildOctree();
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
            }
            else
            {
                Application::GetInstance().renderer->LoadTexture(filePath);
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

    // Load .meta before importing
    std::string metaPath = file_path + ".meta";
    MetaFile meta;

    if (std::filesystem::exists(metaPath)) {
        meta = MetaFile::Load(metaPath);
    }
    else {
        meta = MetaFileManager::GetOrCreateMeta(file_path);
    }

    // Build import flags dynamically from .meta settings
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

    GameObject* rootObj = ProcessNode(scene->mRootNode, scene, directory);

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    glm::mat4 identity(1.0f);
    CalculateBoundingBox(rootObj, minBounds, maxBounds, identity);

    NormalizeModelScale(rootObj, 5.0f);

    // Apply importScale from .meta
    if (meta.uid != 0 && meta.importSettings.importScale != 1.0f) {
        Transform* rootTransform = static_cast<Transform*>(
            rootObj->GetComponent(ComponentType::TRANSFORM));

        if (rootTransform) {
            glm::vec3 currentScale = rootTransform->GetScale();
            glm::vec3 newScale = currentScale * meta.importSettings.importScale;
            rootTransform->SetScale(newScale);
        }
    }

    // Update .meta if necessary
    bool metaChanged = false;

    long long currentTimestamp = MetaFileManager::GetFileTimestamp(file_path);
    if (meta.lastModified != currentTimestamp) {
        std::string modelFilename = MeshImporter::GenerateMeshFilename(
            std::filesystem::path(file_path).stem().string()
        );
        meta.libraryPath = LibraryManager::GetModelPath(modelFilename);
        meta.lastModified = currentTimestamp;
        metaChanged = true;
    }

    if (metaChanged) {
        meta.Save(metaPath);
    }

    aiReleaseImport(scene);

    LOG_CONSOLE("Model loaded successfully");

    return rootObj;
}

GameObject* FileSystem::ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory)
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

        UID meshUID = ProcessMesh(aiMesh, scene);

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

        // Load diffuse textures if available
        if (aiMesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];

            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString texturePath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

                ComponentMaterial* matComponent = static_cast<ComponentMaterial*>(
                    gameObject->GetComponent(ComponentType::MATERIAL));

                if (matComponent == nullptr)
                {
                    matComponent = static_cast<ComponentMaterial*>(
                        gameObject->CreateComponent(ComponentType::MATERIAL));
                }

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
                            break;
                        }
                    }
                }

                if (!loaded)
                {
                    matComponent->CreateCheckerboardTexture();
                }
            }
        }
    }

    // Process child nodes recursively
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        GameObject* child = ProcessNode(node->mChildren[i], scene, directory);
        if (child != nullptr)
        {
            gameObject->AddChild(child);
        }
    }

    return gameObject;
}

UID FileSystem::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
{
    std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());
    std::string fullPath = LibraryManager::GetMeshPath(meshFilename);

    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        LOG_CONSOLE("ERROR: ModuleResources not available");
        return 0;
    }

    // Check if mesh is already registered
    const auto& allResources = resources->GetAllResources();
    for (const auto& pair : allResources) {
        if (pair.second->GetLibraryFile() == fullPath) {
            return pair.first;
        }
    }

    // Load or import mesh
    Mesh mesh;
    bool needsImport = true;

    if (LibraryManager::FileExists(fullPath)) {
        mesh = MeshImporter::LoadFromCustomFormat(meshFilename);
        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            needsImport = false;
        }
    }

    if (needsImport) {
        mesh = MeshImporter::ImportFromAssimp(aiMesh);
        if (!mesh.vertices.empty() && !mesh.indices.empty()) {
            MeshImporter::SaveToCustomFormat(mesh, meshFilename);
        }
        else {
            LOG_CONSOLE("ERROR: Failed to import mesh");
            return 0;
        }
    }

    // Register mesh in ModuleResources
    UID meshUID = resources->GenerateNewUID();
    Resource* newResource = resources->CreateNewResourceWithUID(
        fullPath.c_str(),
        Resource::MESH,
        meshUID
    );

    if (!newResource) {
        LOG_CONSOLE("ERROR: Failed to create resource");
        return 0;
    }

    newResource->libraryFile = fullPath;

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