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

    // Verify Assets folder exists at this level
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

            // Eliminar todos los archivos en Library/ recursivamente
            for (const auto& entry : fs::recursive_directory_iterator(libraryPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                    filesDeleted++;
                }
            }

            LOG_CONSOLE("[LibraryManager] Deleted %d files from Library", filesDeleted);

            // Recrear estructura de carpetas
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

            // Ignorar .meta
            if (extension == ".meta") continue;

            AssetType type = MetaFile::GetAssetType(extension);
            if (type == AssetType::UNKNOWN) continue;

            // Procesar según el tipo
            switch (type) {
            case AssetType::MODEL_FBX: {
                // Verificar si ya está en Library
                const aiScene* scene = aiImportFile(assetPath.string().c_str(),
                    aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);

                if (scene && scene->HasMeshes()) {
                    bool allMeshesExist = true;

                    // Verificar si todas las meshes ya existen
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
                        // Importar todas las meshes del FBX
                        LOG_DEBUG("Importing FBX: %s", assetPath.filename().string().c_str());
                        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                            aiMesh* aiMesh = scene->mMeshes[i];
                            Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);
                            std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());
                            MeshImporter::SaveToCustomFormat(mesh, meshFilename);
                        }
                        processed++;
                    }
                    else {
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
            case AssetType::TEXTURE_DDS: {
                // Verificar si ya está en Library
                std::string filename = TextureImporter::GenerateTextureFilename(assetPath.string());
                std::string fullPath = GetTexturePath(filename);

                if (!FileExists(fullPath)) {
                    LOG_DEBUG("Importing texture: %s", assetPath.filename().string().c_str());
                    TextureData texture = TextureImporter::ImportFromFile(assetPath.string());
                    if (texture.IsValid()) {
                        TextureImporter::SaveToCustomFormat(texture, filename);
                        processed++;
                    }
                    else {
                        LOG_CONSOLE("[LibraryManager] ERROR: Failed to import texture: %s",
                            assetPath.filename().string().c_str());
                        errors++;
                    }
                }
                else {
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