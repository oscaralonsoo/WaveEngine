#include "FileSystem.h"
#define NOMINMAX  
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/cimport.h>
#include <windows.h>
#include <algorithm>
#include <limits>
#include "Application.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "MeshImporter.h"
#include "LibraryManager.h"  
#include "ComponentMaterial.h"

FileSystem::FileSystem() : Module() {}
FileSystem::~FileSystem() {}

bool FileSystem::Awake()
{
    return true;
}

bool FileSystem::Start()
{

    LibraryManager::Initialize();
    LOG_DEBUG("Library structure initialized");
    LOG_CONSOLE("Library folders initialized");

    LOG_DEBUG("Initializing FileSystem module");
    LOG_CONSOLE("FileSystem initialized");

    // Get executable directory
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string execPath(buffer);

    size_t pos = execPath.find_last_of("\\/");
    std::string currentDir = execPath.substr(0, pos);

    // Search for the Assets folder by uploading directories
    std::string assetsPath;
    std::string searchDir = currentDir;
    bool assetsFound = false;

    // Try up to 5 levels up
    for (int i = 0; i < 5 && !assetsFound; i++)
    {
        std::string testPath = searchDir + "\\Assets";

        // Verificar si existe el directorio Assets
        DWORD attribs = GetFileAttributesA(testPath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY))
        {
            assetsPath = testPath;
            assetsFound = true;
            LOG_DEBUG("Assets folder found at: %s", assetsPath.c_str());
            break;
        }

        // Move up a level
        pos = searchDir.find_last_of("\\/");
        if (pos == std::string::npos)
            break;
        searchDir = searchDir.substr(0, pos);
    }

    if (!assetsFound)
    {
        LOG_DEBUG("ERROR: Assets folder not found");
        LOG_CONSOLE("ERROR: Could not locate Assets folder");

        // Create fallback geometry
        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);

        Application::GetInstance().scene->RebuildOctree();

        LOG_CONSOLE("Using fallback geometry - Assets folder not found");
        return true;
    }

    std::string housePath = assetsPath + "\\BakerHouse.fbx";

    LOG_DEBUG("Attempting to load default model: %s", housePath.c_str());
    LOG_CONSOLE("Loading default scene...");

    GameObject* houseModel = LoadFBXAsGameObject(housePath);

    if (houseModel != nullptr)
    {
        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(houseModel);
        LOG_DEBUG("FBX loaded from: %s", housePath.c_str());
        LOG_CONSOLE("Default model loaded: %s", housePath.c_str());

        Application::GetInstance().scene->RebuildOctree();
    }
    else
    {
        LOG_CONSOLE("WARNING: Couldn't load default model");
        LOG_DEBUG("Failed to load default FBX, creating fallback geometry");

        GameObject* pyramidObject = new GameObject("Pyramid");
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(pyramidObject->CreateComponent(ComponentType::MESH));
        Mesh pyramidMesh = Primitives::CreatePyramid();
        meshComp->SetMesh(pyramidMesh);

        GameObject* root = Application::GetInstance().scene->GetRoot();
        root->AddChild(pyramidObject);

        Application::GetInstance().scene->RebuildOctree();

        LOG_DEBUG("Failed to load default FBX, using fallback pyramid. Use drag & drop");
        LOG_CONSOLE("Using fallback geometry");
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
            LOG_DEBUG("Dropped FBX file detected: %s", filePath.c_str());
            LOG_CONSOLE("Loading dropped model...");

            GameObject* loadedModel = LoadFBXAsGameObject(filePath);
            if (loadedModel != nullptr)
            {
                GameObject* root = Application::GetInstance().scene->GetRoot();
                root->AddChild(loadedModel);

                LOG_DEBUG("Model loaded successfully");
                LOG_DEBUG("   Root GameObject: %s", loadedModel->GetName().c_str());
                LOG_DEBUG("   Children: %d", loadedModel->GetChildren().size());
                LOG_CONSOLE("Model loaded successfully: %s", loadedModel->GetName().c_str());

                Application::GetInstance().scene->RebuildOctree();
            }
            else
            {
                LOG_DEBUG("ERROR: Failed to load dropped FBX file");
                LOG_CONSOLE("Failed to load model");
            }
        }
        else if (fileType == DROPPED_TEXTURE)
        {
            LOG_DEBUG("Dropped texture file detected: %s", filePath.c_str());
            // Get all selected objects in the editor
            std::vector<GameObject*> selectedObjects =
                Application::GetInstance().selectionManager->GetSelectedObjects();

            if (!selectedObjects.empty())
            {
                // Apply the texture to all selected objects
                int successCount = 0;
                for (GameObject* obj : selectedObjects)
                {
                    if (ApplyTextureToGameObject(obj, filePath))
                    {
                        successCount++;
                    }
                }

                LOG_DEBUG("✓ Texture applied to %d of %zu selected objects",
                    successCount, selectedObjects.size());
            }
            else
            {
                // Fallback behavior: load the texture globally if no objects are selected
                LOG_CONSOLE("Loading texture...");
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
    LOG_DEBUG("=== Loading FBX with ASSIMP ===");
    LOG_DEBUG("File: %s", file_path.c_str());
    LOG_CONSOLE("Loading model with ASSIMP...");

    unsigned int importFlags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_ValidateDataStructure;

    LOG_DEBUG("ASSIMP import flags: TargetRealtime_MaxQuality | ConvertToLeftHanded");

    const aiScene* scene = aiImportFile(file_path.c_str(), importFlags);

    if (scene == nullptr)
    {
        LOG_DEBUG("ERROR: ASSIMP failed to load file");
        LOG_DEBUG("ASSIMP Error: %s", aiGetErrorString());
        LOG_CONSOLE("ERROR: Failed to load model - %s", aiGetErrorString());
        return nullptr;
    }

    if (!scene->HasMeshes())
    {
        LOG_DEBUG("ERROR: No meshes found in scene");
        LOG_CONSOLE("ERROR: No geometry found in model");
        aiReleaseImport(scene);
        return nullptr;
    }

    std::string directory = file_path.substr(0, file_path.find_last_of("/\\"));

    LOG_DEBUG("=== ASSIMP Scene Information ===");
    LOG_DEBUG("  Meshes: %d", scene->mNumMeshes);
    LOG_DEBUG("  Materials: %d", scene->mNumMaterials);
    LOG_CONSOLE("ASSIMP: Found %d meshes, %d materials", scene->mNumMeshes, scene->mNumMaterials);

    GameObject* rootObj = ProcessNode(scene->mRootNode, scene, directory);

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    glm::mat4 identity(1.0f);
    CalculateBoundingBox(rootObj, minBounds, maxBounds, identity);

    glm::vec3 size = maxBounds - minBounds;
    LOG_DEBUG("Model Dimensions: X=%.2f Y=%.2f Z=%.2f", size.x, size.y, size.z);

    // Normalize scale
    NormalizeModelScale(rootObj, 5.0f);

    aiReleaseImport(scene);

    LOG_DEBUG("=== FBX Loading Complete ===");
    LOG_DEBUG("GameObject hierarchy created successfully");
    LOG_CONSOLE("Model loaded successfully");

    return rootObj;
}

GameObject* FileSystem::ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory)
{
    std::string nodeName = node->mName.C_Str();
    if (nodeName.empty()) nodeName = "Unnamed";

    GameObject* gameObject = new GameObject(nodeName);

    LOG_DEBUG("Processing node: %s", nodeName.c_str());

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

    // Process all meshes for this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        aiMesh* aiMesh = scene->mMeshes[meshIndex];

        LOG_DEBUG("  Processing mesh %d: %s", i, aiMesh->mName.C_Str());

        Mesh mesh = ProcessMesh(aiMesh, scene);

        ComponentMesh* meshComponent = static_cast<ComponentMesh*>(gameObject->CreateComponent(ComponentType::MESH));
        meshComponent->SetMesh(mesh);

        // Load diffuse textures if available
        if (aiMesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];

            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            {
                aiString texturePath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);

                LOG_DEBUG("    Material has texture: %s", texturePath.C_Str());

                ComponentMaterial* matComponent = static_cast<ComponentMaterial*>(gameObject->GetComponent(ComponentType::MATERIAL));

                if (matComponent == nullptr)
                {
                    matComponent = static_cast<ComponentMaterial*>(gameObject->CreateComponent(ComponentType::MATERIAL));
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
                    textureFile
                };

                bool loaded = false;
                for (const auto& path : searchPaths)
                {
                    LOG_DEBUG("      Trying texture at: %s", path.c_str());
                    if (matComponent->LoadTexture(path))
                    {
                        LOG_DEBUG("      Texture loaded successfully from: %s", path.c_str());
                        loaded = true;
                        break;
                    }
                }

                if (!loaded)
                {
                    LOG_DEBUG("      Texture '%s' not found, using checkerboard", fileName.c_str());
                }
            }
        }
    }

    // Recursively process child nodes
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

Mesh FileSystem::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
{
    LOG_DEBUG("=== Processing Mesh with Custom Format ===");
    LOG_DEBUG("Mesh name: %s", aiMesh->mName.C_Str());


    // 1: IMPORT - Assimp -> Our Mesh Structure
    Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);
    LOG_DEBUG("  [IMPORT] Complete");

    //  2: SAVE - Our Mesh -> Custom Binary Format
    std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());

    bool saveSuccess = MeshImporter::SaveToCustomFormat(mesh, meshFilename);

    if (saveSuccess) {
        LOG_DEBUG("  [SAVE] Complete - File: %s", meshFilename.c_str());
        LOG_CONSOLE("Mesh saved to: Library/Meshes/%s", meshFilename.c_str());
    }
    else {
        LOG_DEBUG("  [SAVE] ERROR: Failed to save mesh!");
    }

    //  3: LOAD - Custom Binary Format -> Our Mesh
    Mesh loadedMesh = MeshImporter::LoadFromCustomFormat(meshFilename);
    LOG_DEBUG("  [LOAD] Complete");

    // verification
    if (loadedMesh.vertices.size() != mesh.vertices.size() ||
        loadedMesh.indices.size() != mesh.indices.size())
    {
        LOG_DEBUG("  [WARNING] Loaded mesh differs from original!");
        LOG_DEBUG("    Original: %d vertices, %d indices",
            mesh.vertices.size(), mesh.indices.size());
        LOG_DEBUG("    Loaded:   %d vertices, %d indices",
            loadedMesh.vertices.size(), loadedMesh.indices.size());
    }
    else {
        LOG_DEBUG("  [SUCCESS] Mesh loaded correctly!");
    }

    LOG_DEBUG("      Mesh processed: Vertices: %d, Indices: %d, Triangles: %d",
        loadedMesh.vertices.size(),
        loadedMesh.indices.size(),
        loadedMesh.indices.size() / 3);

    LOG_CONSOLE("  Mesh processed: %d vertices, %d triangles",
        loadedMesh.vertices.size(),
        loadedMesh.indices.size() / 3);

    return loadedMesh;
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

        LOG_DEBUG("Model normalized: %.2f -> scale=%.4f", maxDimension, scale);
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

    // Verificar si el objeto tiene un mesh
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(obj->GetComponent(ComponentType::MESH));

    if (meshComp && meshComp->IsActive() && meshComp->HasMesh())
    {
        // Obtener o crear el componente de material
        ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
            obj->GetComponent(ComponentType::MATERIAL));

        if (matComp == nullptr)
        {
            matComp = static_cast<ComponentMaterial*>(
                obj->CreateComponent(ComponentType::MATERIAL));
        }

        // Cargar la textura
        if (matComp->LoadTexture(texturePath))
        {
            LOG_DEBUG("  ✓ Texture applied to: %s", obj->GetName().c_str());
            applied = true;
        }
        else
        {
            LOG_DEBUG("  ✗ Failed to apply texture to: %s", obj->GetName().c_str());
        }
    }

    // Aplicar recursivamente a los hijos
    for (GameObject* child : obj->GetChildren())
    {
        if (ApplyTextureToGameObject(child, texturePath))
        {
            applied = true;
        }
    }

    return applied;
}