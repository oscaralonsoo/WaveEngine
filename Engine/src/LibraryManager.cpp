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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace fs = std::filesystem;

bool LibraryManager::s_initialized = false;
fs::path LibraryManager::s_projectRoot;

// Function to rotate vertices according to axis configuration
void ApplyAxisConversion(Mesh& mesh, int upAxis, int frontAxis) {
    // upAxis: 0=Y-Up, 1=Z-Up
    // frontAxis: 0=Z-Forward, 1=Y-Forward, 2=X-Forward
    glm::mat4 transform = glm::mat4(1.0f);

    // Up Axis conversions
    if (upAxis == 1) {  // Z-Up (rotate from Y-Up to Z-Up)
        // Rotate -90° around X axis
        transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        LOG_DEBUG("[AxisConversion] Applying Z-Up conversion (rotate -90° on X)");
    }

    // Front Axis conversions
    if (frontAxis == 1) {  // Y-Forward (from Z-Forward)
        // Rotate 90° around X axis
        transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        LOG_DEBUG("[AxisConversion] Applying Y-Forward conversion (rotate 90° on X)");
    }
    else if (frontAxis == 2) {  // X-Forward (from Z-Forward)
        // Rotate -90° around Y axis
        transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        LOG_DEBUG("[AxisConversion] Applying X-Forward conversion (rotate -90° on Y)");
    }

    // Apply transformation to all vertices
    for (auto& vertex : mesh.vertices) {
        glm::vec4 pos = glm::vec4(vertex.position, 1.0f);
        glm::vec4 norm = glm::vec4(vertex.normal, 0.0f);

        pos = transform * pos;
        norm = transform * norm;

        vertex.position = glm::vec3(pos);
        vertex.normal = glm::vec3(norm);
    }
}

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

bool LibraryManager::ReimportAsset(const std::string& assetPath) {
    LOG_CONSOLE("[LibraryManager] Force reimporting: %s", assetPath.c_str());

    if (!fs::exists(assetPath)) {
        LOG_CONSOLE("[LibraryManager] ERROR: Asset not found: %s", assetPath.c_str());
        return false;
    }

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

        TextureData texture = TextureImporter::ImportFromFile(assetPath, meta.importSettings);

        if (texture.IsValid()) {
            std::string filename = std::to_string(meta.uid) + ".texture";
            if (TextureImporter::SaveToCustomFormat(texture, filename)) {
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

        // Delete old mesh files
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
                break;
            }
        }

        LOG_CONSOLE("[LibraryManager] Deleted %d old mesh files", deletedCount);

        // Build import flags from .meta settings
        unsigned int importFlags = aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ValidateDataStructure;

        if (meta.importSettings.generateNormals) {
            importFlags |= aiProcess_GenNormals;
            LOG_DEBUG("[LibraryManager] Applying: Generate Normals");
        }

        if (meta.importSettings.flipUVs) {
            importFlags |= aiProcess_FlipUVs;
            LOG_DEBUG("[LibraryManager] Applying: Flip UVs");
        }

        if (meta.importSettings.optimizeMeshes) {
            importFlags |= aiProcess_OptimizeMeshes;
            LOG_DEBUG("[LibraryManager] Applying: Optimize Meshes");
        }

        LOG_CONSOLE("[LibraryManager] Import flags: 0x%X", importFlags);

        const aiScene* scene = aiImportFile(assetPath.c_str(), importFlags);

        if (scene && scene->HasMeshes()) {

            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[i];
                Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                if (meta.importSettings.importScale != 1.0f) {
                    LOG_DEBUG("[LibraryManager] Applying scale %.3f to mesh %d",
                        meta.importSettings.importScale, i);
                    for (auto& vertex : mesh.vertices) {
                        vertex.position *= meta.importSettings.importScale;
                    }
                }

                unsigned long long meshUID = meta.uid + i;
                std::string meshFilename = std::to_string(meshUID) + ".mesh";

                if (MeshImporter::SaveToCustomFormat(mesh, meshFilename)) {
                    LOG_DEBUG("[LibraryManager] Saved mesh %d (UID: %llu) - Vertices: %zu",
                        i, meshUID, mesh.vertices.size());
                }
                else {
                    LOG_CONSOLE("[LibraryManager] ERROR: Failed to save mesh %d", i);
                }
            }

            meta.lastModified = std::filesystem::last_write_time(assetPath)
                .time_since_epoch().count();
            meta.Save(metaPath);

            aiReleaseImport(scene);

            LOG_CONSOLE("[LibraryManager] FBX reimported successfully: %d meshes", scene->mNumMeshes);
            success = true;
        }
        else {
            LOG_CONSOLE("[LibraryManager] ERROR: Failed to load FBX - %s",
                aiGetErrorString());
        }
        break;
    }

    default:
        LOG_CONSOLE("[LibraryManager] ERROR: Unsupported asset type");
        break;
    }

    return success;
}
void LibraryManager::RegenerateFromAssets() {
    LOG_CONSOLE("[LibraryManager] Scanning Assets and checking for changes...");

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
                }
                else {
                    std::string firstMeshPath = GetMeshPathFromUID(meta.uid);
                    if (!FileExists(firstMeshPath)) {
                        needsImport = true;
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
                    LOG_DEBUG("Importing FBX: %s with %d meshes",
                        assetPath.filename().string().c_str(), scene->mNumMeshes);

                    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                        aiMesh* aiMesh = scene->mMeshes[i];
                        Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                        ApplyAxisConversion(mesh, meta.importSettings.upAxis, meta.importSettings.frontAxis);

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
                }
                else if (!FileExists(fullPath)) {
                    needsImport = true;
                }

                if (!needsImport) {
                    skipped++;
                    break;
                }

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
                        errors++;
                    }
                }
                else {
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

