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

    // Init Library and MetaFile systems
    LibraryManager::Initialize();
    MetaFileManager::Initialize();

    LOG_CONSOLE("[ModuleResources] Library structure created");

    // Scan Assets/ and create .meta files with UIDs
    MetaFileManager::ScanAssets();
    LOG_CONSOLE("[ModuleResources] Asset metadata synchronized");

    // Register all resources from .meta files
    LoadResourcesFromMetaFiles();

    // Import all Assets/ files to Library/
    LOG_CONSOLE("[ModuleResources] Importing assets to Library...");
    LibraryManager::RegenerateFromAssets();

    LOG_CONSOLE("[ModuleResources] Initialized successfully");

    return true;
}

bool ModuleResources::Update() {
    // Hot-reloading could go here
    // Detect changes in Assets/ and reimport automatically
    return true;
}

bool ModuleResources::CleanUp() {
    LOG_CONSOLE("[ModuleResources] Cleaning up...");

    // Unload all resources from memory
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
        LOG_DEBUG("[ModuleResources] Assets folder not found");
        return;
    }

    int registered = 0;
    int skipped = 0;

    // Iterate all files in Assets/
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string assetPath = entry.path().string();
        std::string extension = entry.path().extension().string();

        // Skip .meta files
        if (extension == ".meta") continue;

        // Only process supported types
        AssetType assetType = MetaFile::GetAssetType(extension);
        if (assetType == AssetType::UNKNOWN) continue;

        // Load its .meta
        MetaFile meta = MetaFileManager::LoadMeta(assetPath);

        if (meta.uid == 0) {
            LOG_DEBUG("[ModuleResources] No UID in meta for: %s", assetPath.c_str());
            skipped++;
            continue;
        }

        // Check Library/ file exists
        if (!meta.libraryPath.empty() && !std::filesystem::exists(meta.libraryPath)) {
            LOG_DEBUG("[ModuleResources] Library file missing: %s", meta.libraryPath.c_str());
            skipped++;
            continue;
        }

        // Create resource (don't load into memory yet)
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
            // Just register for now, FBX handled differently
            break;

        default:
            continue;
        }

        if (resource) {
            resource->assetsFile = assetPath;
            resource->libraryFile = meta.libraryPath;
            resources[meta.uid] = resource;
            registered++;

            LOG_DEBUG("[ModuleResources] Registered: UID=%llu, Type=%d, Asset=%s",
                meta.uid, resourceType, assetPath.c_str());
        }
    }

    LOG_CONSOLE("[ModuleResources] Resources registered: %d, skipped: %d",
        registered, skipped);
}

UID ModuleResources::Find(const char* fileInAssets) const {
    // First check loaded resources
    for (const auto& pair : resources) {
        if (pair.second->GetAssetFile() == fileInAssets) {
            return pair.second->GetUID();
        }
    }

    // Not in memory, check .meta
    UID uid = MetaFileManager::GetUIDFromAsset(fileInAssets);
    if (uid != 0) {
        LOG_DEBUG("[ModuleResources] Found UID=%llu in meta file", uid);
    }

    return uid;
}

UID ModuleResources::ImportFile(const char* newFileInAssets) {
    LOG_DEBUG("[ModuleResources] Importing file: %s", newFileInAssets);

    // Load or create .meta (assigns UID if needed)
    MetaFile meta = MetaFileManager::GetOrCreateMeta(newFileInAssets);

    if (meta.uid == 0) {
        LOG_DEBUG("[ModuleResources] ERROR: Failed to get/create UID");
        return 0;
    }

    // Check if already registered
    auto it = resources.find(meta.uid);
    if (it != resources.end()) {
        LOG_DEBUG("[ModuleResources] Resource already exists with UID: %llu", meta.uid);
        return meta.uid;
    }

    // Figure out resource type
    std::filesystem::path path(newFileInAssets);
    std::string extension = path.extension().string();
    Resource::Type type = GetResourceTypeFromExtension(extension);

    if (type == Resource::UNKNOWN) {
        LOG_DEBUG("[ModuleResources] ERROR: Unknown file type: %s", extension.c_str());
        return 0;
    }

    // Create resource using .meta UID
    Resource* resource = CreateNewResourceWithUID(newFileInAssets, type, meta.uid);
    if (!resource) {
        LOG_DEBUG("[ModuleResources] ERROR: Failed to create resource");
        return 0;
    }

    // Run import based on type
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
        LOG_DEBUG("[ModuleResources] ERROR: Import not implemented for this type");
        break;
    }

    if (!importSuccess) {
        LOG_DEBUG("[ModuleResources] ERROR: Import failed");
        delete resource;
        resources.erase(meta.uid);
        return 0;
    }

    // Update .meta with libraryPath
    bool metaNeedsUpdate = false;

    if (meta.libraryPath != resource->libraryFile) {
        meta.libraryPath = resource->libraryFile;
        metaNeedsUpdate = true;
    }

    long long currentTimestamp = MetaFileManager::GetFileTimestamp(newFileInAssets);
    if (meta.lastModified != currentTimestamp) {
        meta.lastModified = currentTimestamp;
        metaNeedsUpdate = true;
    }

    if (metaNeedsUpdate) {
        std::string metaPath = std::string(newFileInAssets) + ".meta";
        meta.Save(metaPath);
        LOG_DEBUG("[ModuleResources] Meta file updated");
    }

    LOG_DEBUG("[ModuleResources] File imported successfully with UID: %llu", meta.uid);

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
        LOG_DEBUG("[ModuleResources] ERROR: Unsupported resource type");
        return nullptr;
    }

    if (resource) {
        resource->assetsFile = assetsFile;
        resource->libraryFile = GenerateLibraryPath(resource);
        resources[uid] = resource;

        LOG_DEBUG("[ModuleResources] Created resource: UID=%llu, Type=%d", uid, type);
    }

    return resource;
}

UID ModuleResources::GenerateNewUID() {
    // Generate unique UID using random + timestamp
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<UID> dis;

    UID uid = 0;

    // Make sure it's unique
    do {
        uid = dis(gen);
    } while (resources.find(uid) != resources.end() || uid == 0);

    return uid;
}

const Resource* ModuleResources::RequestResource(UID uid) const {
    return const_cast<ModuleResources*>(this)->RequestResource(uid);
}

Resource* ModuleResources::RequestResource(UID uid) {
    // Check if resource is already registered
    auto it = resources.find(uid);

    if (it != resources.end()) {
        Resource* resource = it->second;

        // Load into memory if needed
        if (!resource->IsLoadedToMemory()) {
            LOG_DEBUG("[ModuleResources] Loading resource %llu into memory", uid);
            if (!resource->LoadInMemory()) {
                LOG_DEBUG("[ModuleResources] ERROR: Failed to load resource into memory");
                return nullptr;
            }
        }

        // Bump ref count
        resource->referenceCount++;

        LOG_DEBUG("[ModuleResources] Resource %llu requested (refs: %u)",
            uid, resource->referenceCount);

        return resource;
    }

    // Try loading from Library/
    LOG_DEBUG("[ModuleResources] Resource %llu not found, trying to load from library", uid);
    Resource* resource = LoadResourceFromLibrary(uid);

    if (resource) {
        resource->referenceCount++;
        return resource;
    }

    LOG_DEBUG("[ModuleResources] ERROR: Resource %llu not found", uid);
    return nullptr;
}

void ModuleResources::ReleaseResource(UID uid) {
    auto it = resources.find(uid);

    if (it == resources.end()) {
        LOG_DEBUG("[ModuleResources] WARNING: Trying to release unknown resource %llu", uid);
        return;
    }

    Resource* resource = it->second;

    if (resource->referenceCount > 0) {
        resource->referenceCount--;

        LOG_DEBUG("[ModuleResources] Resource %llu released (refs: %u)",
            uid, resource->referenceCount);

        // Unload from memory when no more refs
        if (resource->referenceCount == 0 && resource->IsLoadedToMemory()) {
            LOG_DEBUG("[ModuleResources] Unloading resource %llu from memory", uid);
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
    Resource* resource = nullptr;

    switch (type) {
    case Resource::TEXTURE:
        resource = new ResourceTexture(uid);
        break;

    case Resource::MESH:
        resource = new ResourceMesh(uid);
        break;

        // TODO: Add other types

    default:
        LOG_DEBUG("[ModuleResources] ERROR: Unsupported resource type");
        return nullptr;
    }

    if (resource) {
        resource->assetsFile = assetsFile;
        resource->libraryFile = GenerateLibraryPath(resource);
        resources[uid] = resource;

        LOG_DEBUG("[ModuleResources] Created resource: UID=%llu, Type=%d", uid, type);
    }

    return resource;
}

std::string ModuleResources::GenerateLibraryPath(Resource* resource) {
    // Library/ path generated by resource type
    // For now return empty, filled during import
    return "";
}

Resource* ModuleResources::LoadResourceFromLibrary(UID uid) {
    // TODO: Load from stored metadata
    LOG_DEBUG("[ModuleResources] LoadResourceFromLibrary not yet implemented");
    return nullptr;
}

void ModuleResources::SaveResourceMetadata(Resource* resource) {
    // TODO: Save resource metadata
}

bool ModuleResources::LoadResourceMetadata(UID uid) {
    // TODO: Load resource metadata
    return false;
}

// Import methods
bool ModuleResources::ImportTexture(Resource* resource, const std::string& assetPath) {
    LOG_DEBUG("[ModuleResources] Importing texture: %s", assetPath.c_str());

    // Generate filename in Library
    std::string filename = TextureImporter::GenerateTextureFilename(assetPath);
    std::string libraryPath = LibraryManager::GetTexturePath(filename);

    // Check if already in Library
    if (LibraryManager::FileExists(libraryPath)) {
        LOG_DEBUG("[ModuleResources] Texture already exists in Library, skipping import");
        resource->libraryFile = libraryPath;
        return true;
    }

    // Import from original file
    TextureData textureData = TextureImporter::ImportFromFile(assetPath);

    if (!textureData.IsValid()) {
        LOG_DEBUG("[ModuleResources] ERROR: Failed to import texture data");
        return false;
    }

    // Save to Library
    if (!TextureImporter::SaveToCustomFormat(textureData, filename)) {
        LOG_DEBUG("[ModuleResources] ERROR: Failed to save texture to Library");
        return false;
    }

    resource->libraryFile = libraryPath;

    LOG_DEBUG("[ModuleResources] Texture imported successfully to: %s", libraryPath.c_str());
    return true;
}

bool ModuleResources::ImportMesh(Resource* resource, const std::string& assetPath) {
    LOG_DEBUG("[ModuleResources] ERROR: Direct mesh import not supported, use MODEL import");
    return false;
}

bool ModuleResources::ImportModel(Resource* resource, const std::string& assetPath) {
    LOG_DEBUG("[ModuleResources] Importing model: %s", assetPath.c_str());

    // TODO: Implement FBX model import
    //por ahora lo ahce FileSystem 

    return false;
}

bool ModuleResources::GetResourceInfo(UID uid, std::string& outAssetPath, std::string& outLibraryPath) {
    // First check loaded resources
    auto it = resources.find(uid);
    if (it != resources.end()) {
        outAssetPath = it->second->GetAssetFile();
        outLibraryPath = it->second->GetLibraryFile();
        return true;
    }

    // Not in memory, check .meta files
    outAssetPath = MetaFileManager::GetAssetFromUID(uid);
    if (!outAssetPath.empty()) {
        MetaFile meta = MetaFileManager::LoadMeta(outAssetPath);
        outLibraryPath = meta.libraryPath;
        return true;
    }

    return false;
}

void ModuleResources::RemoveResource(UID uid) {
    auto it = resources.find(uid);

    if (it == resources.end()) {
        LOG_DEBUG("[ModuleResources] Resource %llu not found, nothing to remove", uid);
        return;
    }

    Resource* resource = it->second;

    // Warn if resource still has refs
    if (resource->GetReferenceCount() > 0) {
        LOG_CONSOLE("[ModuleResources] WARNING: Removing resource %llu that still has %u references",
            uid, resource->GetReferenceCount());
    }

    // Unload if loaded
    if (resource->IsLoadedToMemory()) {
        LOG_DEBUG("[ModuleResources] Unloading resource %llu before removal", uid);
        resource->UnloadFromMemory();
    }

    // Cleanup
    delete resource;
    resources.erase(it);

    LOG_CONSOLE("[ModuleResources] Resource %llu removed from system", uid);
}