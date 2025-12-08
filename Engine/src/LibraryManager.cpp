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

    // Get executable directory
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    fs::path execPath(buffer);

    // Get directory containing executable
    fs::path currentDir = execPath.parent_path();

    // Go up 2 levels from executable (build/Debug/ -> build/ -> ProjectRoot/)
    currentDir = currentDir.parent_path().parent_path();

    // Check if Assets folder exists at this level
    fs::path assetsPath = currentDir / "Assets";
    bool assetsFound = fs::exists(assetsPath) && fs::is_directory(assetsPath);

    if (assetsFound) {
        s_projectRoot = currentDir;
        LOG_CONSOLE("[LibraryManager] Project root found at: %s", s_projectRoot.string().c_str());
    }
    else {
        LOG_CONSOLE("[LibraryManager] ERROR: Could not find Assets folder at: %s", assetsPath.string().c_str());
        LOG_DEBUG("Current dir: %s", currentDir.string().c_str());
        return;
    }

    // Create Library root at project level
    fs::path libraryRoot = s_projectRoot / "Library";
    EnsureDirectoryExists(libraryRoot);

    // Create all Library subdirectories
    EnsureDirectoryExists(libraryRoot / "Meshes");
    EnsureDirectoryExists(libraryRoot / "Materials");
    EnsureDirectoryExists(libraryRoot / "Textures");
    EnsureDirectoryExists(libraryRoot / "Models");
    EnsureDirectoryExists(libraryRoot / "Animations");

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

std::string LibraryManager::GetMeshPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Meshes" / filename).string();
}

std::string LibraryManager::GetMaterialPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Materials" / filename).string();
}

std::string LibraryManager::GetTexturePath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Textures" / filename).string();
}

std::string LibraryManager::GetModelPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Models" / filename).string();
}

std::string LibraryManager::GetAnimationPath(const std::string& filename) {
    return (s_projectRoot / "Library" / "Animations" / filename).string();
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

            // Delete all files in Library/ recursively
            for (const auto& entry : fs::recursive_directory_iterator(libraryPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                    filesDeleted++;
                }
            }

            LOG_CONSOLE("[LibraryManager] Deleted %d files from Library", filesDeleted);

            // Recreate folder structure
            Initialize();
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[LibraryManager] ERROR clearing library: %s", e.what());
    }
}

void LibraryManager::RegenerateFromAssets() {
    LOG_CONSOLE("[LibraryManager] Scanning Assets and importing missing files to Library...");

    MetaFileManager::ScanAssets();

    fs::path assetsPath = GetAssetsRoot();
    int processed = 0;
    int skipped = 0;
    int errors = 0;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
            if (!entry.is_regular_file()) continue;

            fs::path assetPath = entry.path();
            std::string extension = assetPath.extension().string();

            // Skip .meta files
            if (extension == ".meta") continue;

            AssetType type = MetaFile::GetAssetType(extension);
            if (type == AssetType::UNKNOWN) continue;

            // Load .meta file
            std::string assetPathStr = assetPath.string();
            MetaFile meta = MetaFileManager::LoadMeta(assetPathStr);

            if (meta.uid == 0) {
                LOG_CONSOLE("[LibraryManager] ERROR: No UID in meta for: %s",
                    assetPath.filename().string().c_str());
                errors++;
                continue;
            }

            // Process by type
            switch (type) {
            case AssetType::MODEL_FBX: {
                // ========== CONSTRUIR FLAGS SEGÚN .META ==========
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

                // Check if already in Library
                const aiScene* scene = aiImportFile(assetPath.string().c_str(), importFlags);

                if (scene && scene->HasMeshes()) {
                    bool allMeshesExist = true;

                    // Check if all meshes already exist
                    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                        aiMesh* aiMesh = scene->mMeshes[i];
                        std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());
                        std::string fullPath = GetMeshPath(meshFilename);

                        if (!FileExists(fullPath)) {
                            allMeshesExist = false;
                            break;
                        }
                    }

                    if (!allMeshesExist) {
                        // Import all meshes from FBX
                        LOG_DEBUG("Importing FBX: %s", assetPath.filename().string().c_str());

                        meta.libraryPaths.clear();

                        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                            aiMesh* aiMesh = scene->mMeshes[i];
                            Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                            // ========== APLICAR importScale A LOS VÉRTICES ==========
                            if (meta.importSettings.importScale != 1.0f) {
                                for (auto& vertex : mesh.vertices) {
                                    vertex.position *= meta.importSettings.importScale;
                                }
                                LOG_DEBUG("[LibraryManager] Applied importScale %.3f to mesh vertices",
                                    meta.importSettings.importScale);
                            }

                            std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());

                            if (MeshImporter::SaveToCustomFormat(mesh, meshFilename)) {
                                std::string fullMeshPath = GetMeshPath(meshFilename);
                                meta.AddLibraryPath(fullMeshPath);
                                LOG_DEBUG("[LibraryManager] Added mesh to meta: %s", fullMeshPath.c_str());
                            }
                        }

                        // Save .meta with all paths
                        if (!meta.libraryPaths.empty()) {
                            std::string metaPath = assetPathStr + ".meta";
                            if (meta.Save(metaPath)) {
                                LOG_DEBUG("[LibraryManager] Updated .meta with %d library paths",
                                    (int)meta.libraryPaths.size());
                            }
                        }

                        processed++;
                    }
                    else {
                        // Already exists, but check .meta has all libraryPaths
                        if (meta.libraryPaths.empty()) {
                            // Rebuild path list
                            for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                                aiMesh* aiMesh = scene->mMeshes[i];
                                std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());
                                std::string fullMeshPath = GetMeshPath(meshFilename);
                                meta.AddLibraryPath(fullMeshPath);
                            }

                            std::string metaPath = assetPathStr + ".meta";
                            meta.Save(metaPath);
                            LOG_DEBUG("[LibraryManager] Fixed empty libraryPaths in .meta with %d paths",
                                (int)meta.libraryPaths.size());
                        }
                        skipped++;
                    }

                    aiReleaseImport(scene);
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
                // Generate filename in Library
                std::string filename = TextureImporter::GenerateTextureFilename(assetPath.string());
                std::string fullPath = GetTexturePath(filename);

                // Check if already in Library
                if (!FileExists(fullPath)) {
                    LOG_DEBUG("Importing texture: %s", assetPath.filename().string().c_str());
                    TextureData texture = TextureImporter::ImportFromFile(assetPath.string());

                    if (texture.IsValid()) {
                        if (TextureImporter::SaveToCustomFormat(texture, filename)) {
                            meta.libraryPath = fullPath;

                            std::string metaPath = assetPathStr + ".meta";
                            if (meta.Save(metaPath)) {
                                LOG_DEBUG("[LibraryManager] Updated .meta with library path: %s",
                                    fullPath.c_str());
                            }

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
                }
                else {
                    // Already exists, but check .meta has libraryPath
                    if (meta.libraryPath.empty()) {
                        meta.libraryPath = fullPath;

                        std::string metaPath = assetPathStr + ".meta";
                        meta.Save(metaPath);
                        LOG_DEBUG("[LibraryManager] Fixed empty libraryPath in .meta: %s",
                            fullPath.c_str());
                    }
                    skipped++;
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

    LOG_CONSOLE("[LibraryManager] Scan complete: %d imported, %d already in Library, %d errors",
        processed, skipped, errors);
}