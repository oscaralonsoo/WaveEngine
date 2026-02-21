#include "LibraryManager.h"
#include "Application.h"
#include "ModuleResources.h"
#include "Log.h"
#include <iostream>
#include <windows.h>
#include "MetaFile.h"
#include "TextureImporter.h"
#include "ModuleLoader.h"
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

    namespace fs = std::filesystem;

    // Obtener ejecutable
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path execPath(buffer);

    // Directorio del ejecutable
    fs::path currentSearchPath = execPath.parent_path();

    // Buscar Assets subiendo niveles
    bool assetsFound = false;
    int maxLevels = 5;

    for (int i = 0; i < maxLevels; ++i) {
        fs::path candidatePath = currentSearchPath / "Assets";

        if (fs::exists(candidatePath) && fs::is_directory(candidatePath)) {
            s_projectRoot = currentSearchPath;
            assetsFound = true;
            LOG_CONSOLE("[LibraryManager] Assets folder found at: %s", candidatePath.string().c_str());
            break;
        }

        // Subir un nivel
        currentSearchPath = currentSearchPath.parent_path();

        // Verificar si llegamos a la raíz
        if (currentSearchPath == currentSearchPath.parent_path()) {
            break;
        }
    }

    if (!assetsFound) {
        LOG_CONSOLE("[LibraryManager] ERROR: Could not find Assets folder");
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

std::string LibraryManager::GetAnimationPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Animations" / filename).string();
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
                meta.fileHash = MetaFileManager::GetFileHash(assetPath);
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

            meta.fileHash = MetaFileManager::GetFileHash(assetPath);
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

            bool needsImport = false;
            std::string libraryPath = "";

            switch (type) {
                case AssetType::MODEL_FBX:
                    libraryPath = GetModelPathFromUID(meta.uid);
                break;
                case AssetType::TEXTURE_PNG:
                case AssetType::TEXTURE_JPG:
                case AssetType::TEXTURE_DDS:
                case AssetType::TEXTURE_TGA: 
                    libraryPath = GetTexturePathFromUID(meta.uid);
                break;
            default:
                break;
            }

            if (libraryPath == "") continue;
            
            if (!FileExists(libraryPath)) {
                needsImport = true;
            }
            else
            {
                if (meta.fileHash != MetaFileManager::GetFileHash(assetPath.generic_string()))
                {
                    needsImport = true;
                }
            }

            if (!needsImport) {
                skipped++;
                continue;
            }

            processed++;
            Application::GetInstance().resources.get()->ImportFile(assetPath.generic_string().c_str(), true);
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[LibraryManager] ERROR during scan: %s", e.what());
    }

    LOG_CONSOLE("[LibraryManager] Scan complete: %d re-imported/new, %d synchronized, %d errors",
        processed, skipped, errors);
}

