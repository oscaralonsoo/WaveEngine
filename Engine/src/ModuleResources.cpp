#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "ResourceMesh.h"
#include "LibraryManager.h"
#include "MetaFile.h"
#include "TextureImporter.h"
#include "MeshImporter.h"
#include "Log.h"
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
    LOG_CONSOLE("[ModuleResources] Cleaning up...");

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

    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string assetPath = entry.path().string();
        std::string extension = entry.path().extension().string();

        // Skip .meta files themselves
        if (extension == ".meta") continue;

        AssetType assetType = MetaFile::GetAssetType(extension);
        if (assetType == AssetType::UNKNOWN) continue;

        // Load meta file IF IT EXISTS 
        std::string metaPath = assetPath + ".meta";
        if (!std::filesystem::exists(metaPath)) {
            skipped++;
            continue; // Skip files without .meta
        }

        MetaFile meta = MetaFile::Load(metaPath);

        if (meta.uid == 0) {
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
            break;

        case AssetType::MODEL_FBX:
            resourceType = Resource::MODEL;
            continue;

        default:
            continue;
        }

        if (resource) {
            resource->assetsFile = assetPath;
            // Generate library path from UID
            resource->libraryFile = LibraryManager::GetTexturePathFromUID(meta.uid);

            resources[meta.uid] = resource;
            registered++;
        }
    }

    LOG_CONSOLE("[ModuleResources] Resources registered: %d, skipped: %d",
        registered, skipped);
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

    default:
        LOG_CONSOLE("ERROR: Unsupported resource type");
        return nullptr;
    }

    if (resource) {
        resource->assetsFile = assetsFile;
        // Generate library path from UID based on type
        if (type == Resource::TEXTURE) {
            resource->libraryFile = LibraryManager::GetTexturePathFromUID(uid);
        }
        else if (type == Resource::MESH) {
            resource->libraryFile = LibraryManager::GetMeshPathFromUID(uid);
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
        resource->libraryFile = libraryPath;
        return true;
    }

    TextureData textureData = TextureImporter::ImportFromFile(assetPath);

    if (!textureData.IsValid()) {
        LOG_CONSOLE("ERROR: Failed to import texture: %s", assetPath.c_str());
        return false;
    }

    if (!TextureImporter::SaveToCustomFormat(textureData, filename)) {
        LOG_CONSOLE("ERROR: Failed to save texture to Library");
        return false;
    }

    resource->libraryFile = libraryPath;
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