#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "ResourceMesh.h"
#include "ResourceShader.h"
#include "LibraryManager.h"
#include "MetaFile.h"
#include "TextureImporter.h"
#include "MeshImporter.h"
#include "Log.h"
#include "ResourceScript.h"
#include "ResourcePrefab.h"
#include <filesystem>
#include <random>

// Resource Implementation
Resource::Resource(UID uid, Type type)
    : uid(uid), type(type), referenceCount(0), loadedInMemory(false) {
}

Resource::~Resource() {
}

// ModuleResources Implementation
ModuleResources::ModuleResources() : Module() {
}

ModuleResources::~ModuleResources() {
}

bool ModuleResources::Awake() {
    return true;
}

bool ModuleResources::Start() {
    LOG_CONSOLE("[ModuleResources] Initializing...");

    LibraryManager::Initialize();
    LOG_CONSOLE("[ModuleResources] Library structure created");

    MetaFileManager::Initialize();
    LOG_CONSOLE("[ModuleResources] Asset metadata synchronized");

    LoadResourcesFromMetaFiles();

    LOG_CONSOLE("[ModuleResources] Importing assets to Library...");
    LibraryManager::RegenerateFromAssets();

    LOG_CONSOLE("[ModuleResources] Initialized successfully");

    return true;
}

bool ModuleResources::Update() {
    return true;
}

bool ModuleResources::CleanUp() {

    for (auto& pair : resources) {
        if (pair.second->IsLoadedToMemory()) {
            pair.second->UnloadFromMemory();
        }
        delete pair.second;
    }

    resources.clear();

    LOG_CONSOLE("[ModuleResources] Cleaned up successfully");
    return true;
}

void ModuleResources::LoadResourcesFromMetaFiles() {
    LOG_CONSOLE("[ModuleResources] Registering resources from meta files...");

    std::string assetsPath = LibraryManager::GetAssetsRoot();

    if (!std::filesystem::exists(assetsPath)) {
        LOG_CONSOLE("ERROR: Assets folder not found");
        return;
    }

    int registered = 0;
    int skipped = 0;


    int prefabsFound = 0;
    int prefabsRegistered = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string assetPath = entry.path().string();
        std::string extension = entry.path().extension().string();

        if (extension == ".meta") continue;

        AssetType assetType = MetaFile::GetAssetType(extension);


        if (extension == ".prefab") {
            LOG_CONSOLE("[ModuleResources] Found .prefab file: %s", assetPath.c_str());
            LOG_CONSOLE("[ModuleResources] AssetType: %d (7=PREFAB expected)", (int)assetType);
        }

        if (assetType == AssetType::UNKNOWN) continue;

        std::string metaPath = assetPath + ".meta";
        if (!std::filesystem::exists(metaPath)) {

            if (extension == ".prefab") {
                LOG_CONSOLE("[ModuleResources] No .meta file for prefab: %s", assetPath.c_str());
            }
            skipped++;
            continue;
        }

        MetaFile meta = MetaFile::Load(metaPath);

        if (meta.uid == 0) {

            if (extension == ".prefab") {
                LOG_CONSOLE("[ModuleResources] Invalid UID in .meta for prefab: %s", assetPath.c_str());
            }
            skipped++;
            continue;
        }

        Resource* resource = nullptr;
        Resource::Type resourceType = Resource::UNKNOWN;

        switch (assetType) {
        case AssetType::TEXTURE_PNG:
        case AssetType::TEXTURE_JPG:
        case AssetType::TEXTURE_DDS:
        case AssetType::TEXTURE_TGA:
            resourceType = Resource::TEXTURE;
            resource = new ResourceTexture(meta.uid);
            if (resource) {
                resource->SetAssetFile(assetPath);
                resource->SetLibraryFile(LibraryManager::GetTexturePathFromUID(meta.uid));
                resources[meta.uid] = resource;
                registered++;
            }
            break;

        case AssetType::MODEL_FBX:
            for (unsigned int i = 0; i < 100; ++i) {
                UID meshUID = meta.uid + i;
                std::string meshPath = LibraryManager::GetMeshPathFromUID(meshUID);

                if (!std::filesystem::exists(meshPath)) {
                    break;
                }

                ResourceMesh* meshResource = new ResourceMesh(meshUID);
                meshResource->SetAssetFile(assetPath);
                meshResource->SetLibraryFile(meshPath);
                resources[meshUID] = meshResource;
                registered++;
            }
            break;

        case AssetType::SHADER_GLSL:
            resourceType = Resource::SHADER;
            resource = new ResourceShader(meta.uid);
            if (resource) {
                resource->SetAssetFile(assetPath);
                resource->SetLibraryFile("");
                resources[meta.uid] = resource;
                registered++;
            }
            break;

        case AssetType::SCRIPT_LUA:
            resourceType = Resource::SCRIPT;
            resource = new ResourceScript(meta.uid);
            if (resource) {
                resource->SetAssetFile(assetPath);
                resource->SetLibraryFile(assetPath);
                resources[meta.uid] = resource;
                registered++;
            }
            break;

        case AssetType::PREFAB:
            prefabsFound++;
            LOG_CONSOLE("[ModuleResources] Processing prefab: %s", assetPath.c_str());
            LOG_CONSOLE("[ModuleResources] Prefab UID: %llu", meta.uid);

            resourceType = Resource::PREFAB;
            resource = new ResourcePrefab(meta.uid);  

            if (resource) {
                resource->SetAssetFile(assetPath);
                resource->SetLibraryFile(assetPath);
                resources[meta.uid] = resource;
                prefabsRegistered++;
                registered++;
            }
            break;
        default:
            continue;
        }
    }

    LOG_CONSOLE("[ModuleResources] Resources registered: %d, skipped: %d",
        registered, skipped);


    LOG_CONSOLE("[ModuleResources] Prefabs found: %d, registered: %d", prefabsFound, prefabsRegistered);


    LOG_CONSOLE("[ModuleResources] LISTING ALL PREFABS");
    for (const auto& [uid, res] : resources) {
        if (res->GetType() == Resource::PREFAB) {
            LOG_CONSOLE("[ModuleResources]   - Prefab: %s (UID: %llu)",
                res->GetAssetFile().c_str(), uid);
        }
    }
    LOG_CONSOLE("[ModuleResources] === END OF PREFAB LIST ===");
}

UID ModuleResources::Find(const char* fileInAssets) const {
    for (const auto& pair : resources) {
        if (pair.second->GetAssetFile() == fileInAssets) {
            return pair.second->GetUID();
        }
    }

    UID uid = MetaFileManager::GetUIDFromAsset(fileInAssets);
    return uid;
}

UID ModuleResources::ImportFile(const char* newFileInAssets) {
    MetaFile meta = MetaFileManager::GetOrCreateMeta(newFileInAssets);

    if (meta.uid == 0) {
        LOG_CONSOLE("ERROR: Failed to create UID for: %s", newFileInAssets);
        return 0;
    }

    auto it = resources.find(meta.uid);
    if (it != resources.end()) {
        return meta.uid;
    }

    std::filesystem::path path(newFileInAssets);
    std::string extension = path.extension().string();
    Resource::Type type = GetResourceTypeFromExtension(extension);

    if (type == Resource::UNKNOWN) {
        LOG_CONSOLE("ERROR: Unknown file type: %s", extension.c_str());
        return 0;
    }

    Resource* resource = CreateNewResourceWithUID(newFileInAssets, type, meta.uid);
    if (!resource) {
        LOG_CONSOLE("ERROR: Failed to create resource");
        return 0;
    }

    bool importSuccess = false;

    switch (type) {
    case Resource::TEXTURE: {
        importSuccess = ImportTexture(resource, newFileInAssets);
        break;
    }
    case Resource::MESH: {
        importSuccess = ImportMesh(resource, newFileInAssets);
        break;
    }
    case Resource::MODEL: {
        importSuccess = ImportModel(resource, newFileInAssets);
        break;
    }
    case Resource::SHADER: {
        importSuccess = true; 
        break;
    }
    case Resource::SCRIPT: {
        importSuccess = ImportScript(resource, newFileInAssets);
        break;
    }
    case Resource::PREFAB: {
        importSuccess = ImportPrefab(resource, newFileInAssets);
        break;
    }
    default:
        LOG_CONSOLE("ERROR: Import not implemented for this type");
        break;
    }

    if (!importSuccess) {
        LOG_CONSOLE("ERROR: Import failed for: %s", newFileInAssets);
        delete resource;
        resources.erase(meta.uid);
        return 0;
    }

    // Update timestamp in meta
    long long currentTimestamp = MetaFileManager::GetFileTimestamp(newFileInAssets);
    if (meta.lastModified != currentTimestamp) {
        meta.lastModified = currentTimestamp;
        std::string metaPath = std::string(newFileInAssets) + ".meta";
        meta.Save(metaPath);
    }

    return meta.uid;
}

Resource* ModuleResources::CreateNewResourceWithUID(const char* assetsFile, Resource::Type type, UID uid) {
    Resource* resource = nullptr;

    switch (type) {
    case Resource::TEXTURE:
        resource = new ResourceTexture(uid);
        break;

    case Resource::MESH:
        resource = new ResourceMesh(uid);
        break;

    case Resource::SHADER:
        resource = new ResourceShader(uid);
        break;
    case Resource::SCRIPT:  
        resource = new ResourceScript(uid);
        break;
    case Resource::PREFAB:
        resource = new ResourcePrefab(uid);
        break;
    default:
        LOG_CONSOLE("ERROR: Unsupported resource type");
        return nullptr;
    }

    if (resource) {
        resource->SetAssetFile(assetsFile);

        // Scripts NO van a Library
        if (type == Resource::SCRIPT || type == Resource::PREFAB) {
            resource->SetLibraryFile(assetsFile);
        }
        else if (type == Resource::TEXTURE) {
            resource->SetLibraryFile(LibraryManager::GetTexturePathFromUID(uid));
        }
        else if (type == Resource::MESH) {
            resource->SetLibraryFile(LibraryManager::GetMeshPathFromUID(uid));
        }

        resources[uid] = resource;
    }

    return resource;
}

UID ModuleResources::GenerateNewUID() {
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<UID> dis;

    UID uid = 0;

    do {
        uid = dis(gen);
    } while (resources.find(uid) != resources.end() || uid == 0);

    return uid;
}

const Resource* ModuleResources::RequestResource(UID uid) const {
    return const_cast<ModuleResources*>(this)->RequestResource(uid);
}

Resource* ModuleResources::RequestResource(UID uid) {
    auto it = resources.find(uid);

    if (it != resources.end()) {
        Resource* resource = it->second;

        if (!resource->IsLoadedToMemory()) {
            if (!resource->LoadInMemory()) {
                LOG_CONSOLE("ERROR: Failed to load resource %llu into memory", uid);
                return nullptr;
            }
        }

        resource->referenceCount++;
        return resource;
    }

    Resource* resource = LoadResourceFromLibrary(uid);

    if (resource) {
        resource->referenceCount++;
        return resource;
    }

    LOG_CONSOLE("ERROR: Resource %llu not found", uid);
    return nullptr;
}

void ModuleResources::ReleaseResource(UID uid) {
    auto it = resources.find(uid);

    if (it == resources.end()) {
        return;
    }

    Resource* resource = it->second;

    if (resource->referenceCount > 0) {
        resource->referenceCount--;

        if (resource->referenceCount == 0 && resource->IsLoadedToMemory()) {
            resource->UnloadFromMemory();
        }
    }
}

Resource::Type ModuleResources::GetResourceTypeFromExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".dds" || ext == ".tga") {
        return Resource::TEXTURE;
    }

    if (ext == ".fbx" || ext == ".obj") {
        return Resource::MODEL;
    }

    if (ext == ".mesh") {
        return Resource::MESH;
    }

    if (ext == ".glsl") {
        return Resource::SHADER;
    }
    if (ext == ".lua") {  
        return Resource::SCRIPT;
    }
    if (ext == ".prefab") { 
        return Resource::PREFAB;
    }

    return Resource::UNKNOWN;
}
Resource* ModuleResources::CreateNewResource(const char* assetsFile, Resource::Type type) {
    UID uid = GenerateNewUID();
    return CreateNewResourceWithUID(assetsFile, type, uid);
}

std::string ModuleResources::GenerateLibraryPath(Resource* resource) {
    if (!resource) return "";

    switch (resource->GetType()) {
    case Resource::TEXTURE:
        return LibraryManager::GetTexturePathFromUID(resource->GetUID());
    case Resource::MESH:
        return LibraryManager::GetMeshPathFromUID(resource->GetUID());
    case Resource::MODEL:
        return LibraryManager::GetModelPathFromUID(resource->GetUID());
    case Resource::MATERIAL:
        return LibraryManager::GetMaterialPathFromUID(resource->GetUID());
    case Resource::ANIMATION:
        return LibraryManager::GetAnimationPathFromUID(resource->GetUID());
    case Resource::SHADER:
        return ""; // Shaders load directly from assets
    default:
        return "";
    }
}

Resource* ModuleResources::LoadResourceFromLibrary(UID uid) {
    return nullptr;
}

void ModuleResources::SaveResourceMetadata(Resource* resource) {
}

bool ModuleResources::LoadResourceMetadata(UID uid) {
    return false;
}

bool ModuleResources::ImportTexture(Resource* resource, const std::string& assetPath) {
    std::string filename = std::to_string(resource->GetUID()) + ".texture";
    std::string libraryPath = LibraryManager::GetTexturePathFromUID(resource->GetUID());

    if (LibraryManager::FileExists(libraryPath)) {
        resource->SetLibraryFile(libraryPath);
        return true;
    }

    std::string metaPath = assetPath + ".meta";
    MetaFile meta;

    if (std::filesystem::exists(metaPath)) {
        meta = MetaFile::Load(metaPath);
    }
    else {
        // If .meta doesn't exist, create one with appropriate defaults
        meta = MetaFileManager::GetOrCreateMeta(assetPath);
        std::filesystem::path path(assetPath);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        // TGA comes flipped by default, so DON'T flip
        // PNG/JPG come normal, so DO flip for OpenGL
        if (extension == ".tga") {
            meta.importSettings.flipUVs = false;  // TGA already comes flipped
        }
        else {
            meta.importSettings.flipUVs = true;   // PNG/JPG need flip
        }

        meta.Save(metaPath);
    }

    TextureData textureData = TextureImporter::ImportFromFile(assetPath, meta.importSettings);

    if (!textureData.IsValid()) {
        LOG_CONSOLE("ERROR: Failed to import texture: %s", assetPath.c_str());
        return false;
    }

    if (!TextureImporter::SaveToCustomFormat(textureData, filename)) {
        LOG_CONSOLE("ERROR: Failed to save texture to Library");
        return false;
    }

    resource->SetLibraryFile(libraryPath);
    return true;
}

bool ModuleResources::ImportMesh(Resource* resource, const std::string& assetPath) {
    LOG_CONSOLE("ERROR: Direct mesh import not supported");
    return false;
}

bool ModuleResources::ImportModel(Resource* resource, const std::string& assetPath) {
    return false;
}

bool ModuleResources::GetResourceInfo(UID uid, std::string& outAssetPath, std::string& outLibraryPath) {
    auto it = resources.find(uid);
    if (it != resources.end()) {
        outAssetPath = it->second->GetAssetFile();
        outLibraryPath = it->second->GetLibraryFile();
        return true;
    }

    outAssetPath = MetaFileManager::GetAssetFromUID(uid);
    if (!outAssetPath.empty()) {
        // Generate library path from UID and asset type
        MetaFile meta = MetaFileManager::LoadMeta(outAssetPath);

        switch (meta.type) {
        case AssetType::TEXTURE_PNG:
        case AssetType::TEXTURE_JPG:
        case AssetType::TEXTURE_DDS:
        case AssetType::TEXTURE_TGA:
            outLibraryPath = LibraryManager::GetTexturePathFromUID(uid);
            break;
        case AssetType::MODEL_FBX:
            outLibraryPath = LibraryManager::GetMeshPathFromUID(uid);
            break;
        default:
            outLibraryPath = "";
            break;
        }
        return true;
    }

    return false;
}

void ModuleResources::RemoveResource(UID uid) {
    auto it = resources.find(uid);

    if (it == resources.end()) {
        return;
    }

    Resource* resource = it->second;

    if (resource->GetReferenceCount() > 0) {
        LOG_CONSOLE("[ModuleResources] WARNING: Removing resource %llu that still has %u references",
            uid, resource->GetReferenceCount());
    }

    if (resource->IsLoadedToMemory()) {
        resource->UnloadFromMemory();
    }

    delete resource;
    resources.erase(it);

    LOG_CONSOLE("[ModuleResources] Resource %llu removed from system", uid);
}

bool ModuleResources::IsResourceLoaded(UID uid) const {
    auto it = resources.find(uid);
    if (it == resources.end()) {
        return false;
    }

    return it->second->IsLoadedToMemory();
}

unsigned int ModuleResources::GetResourceReferenceCount(UID uid) const {
    auto it = resources.find(uid);
    if (it == resources.end()) {
        return 0;
    }

    return it->second->GetReferenceCount();
}

const Resource* ModuleResources::GetResource(UID uid) const {
    auto it = resources.find(uid);
    if (it == resources.end()) {
        return nullptr;
    }

    return it->second;
}

bool ModuleResources::ImportScript(Resource* resource, const std::string& assetPath) {
    // Scripts don't need importing - they stay in Assets/
    // Just verify the file exists
    if (!std::filesystem::exists(assetPath)) {
        LOG_CONSOLE("ERROR: Script file not found: %s", assetPath.c_str());
        return false;
    }

    resource->SetAssetFile(assetPath);
    resource->SetLibraryFile(assetPath);  // Scripts reference themselves

    LOG_CONSOLE("[ModuleResources] Script registered: %s (UID: %llu)",
        assetPath.c_str(), resource->GetUID());

    return true;
}

bool ModuleResources::ImportPrefab(Resource* resource, const std::string& assetPath) {
    if (!std::filesystem::exists(assetPath)) {
        LOG_CONSOLE("ERROR: Prefab file not found: %s", assetPath.c_str());
        return false;
    }

    // Try to parse JSON to verify it's valid
    std::ifstream file(assetPath);
    if (!file.is_open()) {
        LOG_CONSOLE("ERROR: Cannot open prefab file: %s", assetPath.c_str());
        return false;
    }

    try {
        nlohmann::json testParse;
        file >> testParse;
        file.close();
    }
    catch (const std::exception& e) {
        LOG_CONSOLE("ERROR: Invalid prefab JSON: %s - %s", assetPath.c_str(), e.what());
        file.close();
        return false;
    }

    resource->SetAssetFile(assetPath);
    resource->SetLibraryFile(assetPath);  // Prefabs reference themselves

    LOG_CONSOLE("[ModuleResources] Prefab registered: %s (UID: %llu)",
        assetPath.c_str(), resource->GetUID());

    return true;
}