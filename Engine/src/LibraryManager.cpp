#include "LibraryManager.h"
#include "Log.h"
#include <iostream>
#include <windows.h>
#include "MetaFile.h"
#include "TextureImporter.h"
#include "FileSystem.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h> 
#include <assimp/cimport.h>
#include "MeshImporter.h"

namespace fs = std::filesystem;

bool LibraryManager::s_initialized = false;
fs::path LibraryManager::s_projectRoot;

void LibraryManager::Initialize() {
    if (s_initialized) {
        return;
    }

    LOG_CONSOLE("[LibraryManager] Initializing library structure...");

    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path execPath(buffer);

    fs::path currentDir = execPath.parent_path();
    currentDir = currentDir.parent_path().parent_path();

    fs::path assetsPath = currentDir / "Assets";
    bool assetsFound = fs::exists(assetsPath) && fs::is_directory(assetsPath);

    if (assetsFound) {
        s_projectRoot = currentDir;
        LOG_CONSOLE("[LibraryManager] Project root found at: %s", s_projectRoot.string().c_str());
    }
    else {
        LOG_CONSOLE("[LibraryManager] ERROR: Could not find Assets folder at: %s", assetsPath.string().c_str());
        return;
    }

    fs::path libraryRoot = s_projectRoot / "Library";
    EnsureDirectoryExists(libraryRoot);

    EnsureDirectoryExists(libraryRoot / "Meshes");
    EnsureDirectoryExists(libraryRoot / "Materials");
    EnsureDirectoryExists(libraryRoot / "Textures");
    EnsureDirectoryExists(libraryRoot / "Models");
    EnsureDirectoryExists(libraryRoot / "Animations");
    EnsureDirectoryExists(libraryRoot / "TempScene");

    s_initialized = true;
    LOG_CONSOLE("[LibraryManager] Library initialized successfully");
}

void LibraryManager::EnsureDirectoryExists(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
            LOG_DEBUG("Created directory: %s", path.string().c_str());
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[LibraryManager] ERROR creating directory %s: %s", path.string().c_str(), e.what());
    }
}

bool LibraryManager::IsInitialized() {
    return s_initialized;
}

std::string LibraryManager::GetLibraryRoot() {
    return (s_projectRoot / "Library").string();
}

std::string LibraryManager::GetAssetsRoot() {
    return (s_projectRoot / "Assets").string();
}

// UID-based methods (primary)
std::string LibraryManager::GetMeshPathFromUID(unsigned long long uid) {
    std::string filename = std::to_string(uid) + ".mesh";
    return (s_projectRoot / "Library" / "Meshes" / filename).string();
}

std::string LibraryManager::GetMaterialPathFromUID(unsigned long long uid) {
    std::string filename = std::to_string(uid) + ".mat";
    return (s_projectRoot / "Library" / "Materials" / filename).string();
}

std::string LibraryManager::GetTexturePathFromUID(unsigned long long uid) {
    std::string filename = std::to_string(uid) + ".texture";
    return (s_projectRoot / "Library" / "Textures" / filename).string();
}

std::string LibraryManager::GetModelPathFromUID(unsigned long long uid) {
    std::string filename = std::to_string(uid) + ".model";
    return (s_projectRoot / "Library" / "Models" / filename).string();
}

std::string LibraryManager::GetAnimationPathFromUID(unsigned long long uid) {
    std::string filename = std::to_string(uid) + ".anim";
    return (s_projectRoot / "Library" / "Animations" / filename).string();
}

// Legacy methods (for backward compatibility)
std::string LibraryManager::GetMeshPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Meshes" / filename).string();
}

std::string LibraryManager::GetTexturePath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Textures" / filename).string();
}

std::string LibraryManager::GetModelPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Models" / filename).string();
}

bool LibraryManager::FileExists(const fs::path& path) {
    return fs::exists(path);
}

void LibraryManager::ClearLibrary() {
    fs::path libraryPath = s_projectRoot / "Library";

    LOG_CONSOLE("[LibraryManager] Clearing Library folder...");

    try {
        if (fs::exists(libraryPath)) {
            int filesDeleted = 0;

            for (const auto& entry : fs::recursive_directory_iterator(libraryPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                    filesDeleted++;
                }
            }

            LOG_CONSOLE("[LibraryManager] Deleted %d files from Library", filesDeleted);
            Initialize();
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[LibraryManager] ERROR clearing library: %s", e.what());
    }
}

void LibraryManager::RegenerateFromAssets() {
    LOG_CONSOLE("[LibraryManager] Scanning Assets and checking for changes (Timestamp check)...");

    fs::path assetsPath = GetAssetsRoot();
    int processed = 0;
    int skipped = 0;
    int errors = 0;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
            if (!entry.is_regular_file()) continue;

            fs::path assetPath = entry.path();
            std::string extension = assetPath.extension().string();

            if (extension == ".meta") continue;

            AssetType type = MetaFile::GetAssetType(extension);
            if (type == AssetType::UNKNOWN) continue;

            std::string assetPathStr = assetPath.string();
            std::string metaPathStr = assetPathStr + ".meta";

            if (!fs::exists(metaPathStr)) continue;

            MetaFile meta = MetaFileManager::LoadMeta(assetPathStr);

            if (meta.uid == 0) {
                LOG_CONSOLE("[LibraryManager] ERROR: No UID in meta for: %s",
                    assetPath.filename().string().c_str());
                errors++;
                continue;
            }

            auto assetTime = fs::last_write_time(assetPath);
            long long currentAssetTimestamp = assetTime.time_since_epoch().count();

            const long long tolerance = 20000000000;

            bool assetModified = (std::abs(currentAssetTimestamp - meta.lastModified) > tolerance);

            switch (type) {
            case AssetType::MODEL_FBX: {
                bool needsImport = false;

                if (assetModified) {
                    needsImport = true;
                    LOG_CONSOLE("[LibraryManager] Asset modified: %s", assetPath.filename().string().c_str());
                }
                else {
                    std::string firstMeshPath = GetMeshPathFromUID(meta.uid);
                    if (!FileExists(firstMeshPath)) {
                        needsImport = true;
                        LOG_CONSOLE("[LibraryManager] Missing library file for UID: %llu", meta.uid);
                    }
                }

                if (!needsImport) {
                    skipped++;
                    break;
                }

                unsigned int importFlags = aiProcess_Triangulate |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_ValidateDataStructure;

                if (meta.importSettings.generateNormals) importFlags |= aiProcess_GenNormals;
                if (meta.importSettings.flipUVs) importFlags |= aiProcess_FlipUVs;
                if (meta.importSettings.optimizeMeshes) importFlags |= aiProcess_OptimizeMeshes;

                const aiScene* scene = aiImportFile(assetPath.string().c_str(), importFlags);

                if (scene && scene->HasMeshes()) {
                    LOG_DEBUG("Re-Importing FBX: %s", assetPath.filename().string().c_str());

                    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                        aiMesh* aiMesh = scene->mMeshes[i];
                        Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                        if (meta.importSettings.importScale != 1.0f) {
                            for (auto& vertex : mesh.vertices) {
                                vertex.position *= meta.importSettings.importScale;
                            }
                        }

                        unsigned long long meshUID = meta.uid + i;

                        std::string meshFilename = std::to_string(meshUID) + ".mesh";
                        MeshImporter::SaveToCustomFormat(mesh, meshFilename);
                    }

                    meta.lastModified = currentAssetTimestamp;
                    meta.Save(metaPathStr);

                    aiReleaseImport(scene);
                    processed++;
                }
                else {
                    LOG_CONSOLE("[LibraryManager] ERROR: Failed to load FBX: %s",
                        assetPath.filename().string().c_str());
                    errors++;
                }
                break;
            }

            case AssetType::TEXTURE_PNG:
            case AssetType::TEXTURE_JPG:
            case AssetType::TEXTURE_DDS:
            case AssetType::TEXTURE_TGA: {
                std::string fullPath = GetTexturePathFromUID(meta.uid);

                bool needsImport = false;

                if (assetModified) {
                    needsImport = true;
                    LOG_CONSOLE("[LibraryManager] Asset modified: %s", assetPath.filename().string().c_str());
                }
                else if (!FileExists(fullPath)) {
                    needsImport = true;
                    LOG_CONSOLE("[LibraryManager] Missing library file for UID: %llu", meta.uid);
                }

                if (!needsImport) {
                    skipped++;
                    break;
                }

                LOG_DEBUG("Importing texture: %s", assetPath.filename().string().c_str());

                TextureData texture = TextureImporter::ImportFromFile(
                    assetPath.string(),
                    meta.importSettings  
                );

                if (texture.IsValid()) {
                    std::string filename = std::to_string(meta.uid) + ".texture";
                    if (TextureImporter::SaveToCustomFormat(texture, filename)) {
                        meta.lastModified = currentAssetTimestamp;
                        meta.Save(metaPathStr);
                        processed++;
                    }
                    else {
                        LOG_CONSOLE("[LibraryManager] ERROR: Failed to save texture: %s",
                            assetPath.filename().string().c_str());
                        errors++;
                    }
                }
                else {
                    LOG_CONSOLE("[LibraryManager] ERROR: Failed to import texture: %s",
                        assetPath.filename().string().c_str());
                    errors++;
                }
                break;
            }

            default:
                break;
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[LibraryManager] ERROR during scan: %s", e.what());
    }

    LOG_CONSOLE("[LibraryManager] Scan complete: %d re-imported/new, %d synchronized, %d errors",
        processed, skipped, errors);
}

bool LibraryManager::ReimportAsset(const std::string& assetPath) {
    LOG_CONSOLE("[LibraryManager] Force reimporting: %s", assetPath.c_str());

    if (!fs::exists(assetPath)) {
        LOG_CONSOLE("[LibraryManager] ERROR: Asset not found: %s", assetPath.c_str());
        return false;
    }

    // Cargar .meta
    std::string metaPath = assetPath + ".meta";
    if (!fs::exists(metaPath)) {
        LOG_CONSOLE("[LibraryManager] ERROR: No .meta file found");
        return false;
    }

    MetaFile meta = MetaFile::Load(metaPath);
    if (meta.uid == 0) {
        LOG_CONSOLE("[LibraryManager] ERROR: Invalid UID in .meta");
        return false;
    }

    AssetType type = meta.type;
    bool success = false;

    switch (type) {
    case AssetType::TEXTURE_PNG:
    case AssetType::TEXTURE_JPG:
    case AssetType::TEXTURE_DDS:
    case AssetType::TEXTURE_TGA: {
        LOG_CONSOLE("[LibraryManager] Reimporting texture...");

        // Borrar archivo viejo en Library
        std::string libraryPath = GetTexturePathFromUID(meta.uid);
        if (fs::exists(libraryPath)) {
            try {
                fs::remove(libraryPath);
                LOG_DEBUG("[LibraryManager] Deleted old library file: %s", libraryPath.c_str());
            }
            catch (const std::exception& e) {
                LOG_CONSOLE("[LibraryManager] ERROR deleting old file: %s", e.what());
            }
        }

        // Reimportar con settings del .meta
        TextureData texture = TextureImporter::ImportFromFile(assetPath, meta.importSettings);

        if (texture.IsValid()) {
            std::string filename = std::to_string(meta.uid) + ".texture";
            if (TextureImporter::SaveToCustomFormat(texture, filename)) {
                // Actualizar timestamp
                meta.lastModified = std::filesystem::last_write_time(assetPath)
                    .time_since_epoch().count();
                meta.Save(metaPath);

                LOG_CONSOLE("[LibraryManager] Texture reimported successfully");
                success = true;
            }
            else {
                LOG_CONSOLE("[LibraryManager] ERROR: Failed to save texture");
            }
        }
        else {
            LOG_CONSOLE("[LibraryManager] ERROR: Failed to import texture");
        }
        break;
    }

    case AssetType::MODEL_FBX: {
        LOG_CONSOLE("[LibraryManager] Reimporting FBX model...");

        // Borrar todas las meshes viejas
        int deletedCount = 0;
        for (int i = 0; i < 100; i++) {
            unsigned long long meshUID = meta.uid + i;
            std::string meshPath = GetMeshPathFromUID(meshUID);

            if (fs::exists(meshPath)) {
                try {
                    fs::remove(meshPath);
                    deletedCount++;
                }
                catch (const std::exception& e) {
                    LOG_CONSOLE("[LibraryManager] ERROR deleting mesh %d: %s", i, e.what());
                }
            }
            else {
                break; // No más meshes
            }
        }

        LOG_CONSOLE("[LibraryManager] Deleted %d old mesh files", deletedCount);

        // Reimportar FBX con settings del .meta
        unsigned int importFlags = aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ValidateDataStructure;

        if (meta.importSettings.generateNormals) importFlags |= aiProcess_GenNormals;
        if (meta.importSettings.flipUVs) importFlags |= aiProcess_FlipUVs;
        if (meta.importSettings.optimizeMeshes) importFlags |= aiProcess_OptimizeMeshes;

        const aiScene* scene = aiImportFile(assetPath.c_str(), importFlags);

        if (scene && scene->HasMeshes()) {
            LOG_CONSOLE("[LibraryManager] Processing %d meshes...", scene->mNumMeshes);

            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                // Aplicar escala
                if (meta.importSettings.importScale != 1.0f) {
                    for (auto& vertex : mesh.vertices) {
                        vertex.position *= meta.importSettings.importScale;
                    }
                }

                unsigned long long meshUID = meta.uid + i;
                std::string meshFilename = std::to_string(meshUID) + ".mesh";

                if (MeshImporter::SaveToCustomFormat(mesh, meshFilename)) {
                    LOG_DEBUG("[LibraryManager] Saved mesh %d (UID: %llu)", i, meshUID);
                }
                else {
                    LOG_CONSOLE("[LibraryManager] ERROR: Failed to save mesh %d", i);
                }
            }

            // Actualizar timestamp
            meta.lastModified = std::filesystem::last_write_time(assetPath)
                .time_since_epoch().count();
            meta.Save(metaPath);

            aiReleaseImport(scene);

            LOG_CONSOLE("[LibraryManager] FBX reimported successfully: %d meshes", scene->mNumMeshes);
            success = true;
        }
        else {
            LOG_CONSOLE("[LibraryManager] ERROR: Failed to load FBX");
        }
        break;
    }

    default:
        LOG_CONSOLE("[LibraryManager] ERROR: Unsupported asset type");
        break;
    }

    return success;
}