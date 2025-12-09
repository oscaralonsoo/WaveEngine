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

            // Skip .meta files (procesamos el asset, no su meta directamente)
            if (extension == ".meta") continue;

            AssetType type = MetaFile::GetAssetType(extension);
            if (type == AssetType::UNKNOWN) continue;

            // Load .meta file
            std::string assetPathStr = assetPath.string();
            std::string metaPathStr = assetPathStr + ".meta";

            // Si no hay meta, probablemente es un archivo nuevo, lo saltamos por seguridad o lo creamos
            // (Tu lógica actual asume que existe, así que seguimos esa línea)
            if (!fs::exists(metaPathStr)) continue;

            MetaFile meta = MetaFileManager::LoadMeta(assetPathStr);

            if (meta.uid == 0) {
                LOG_CONSOLE("[LibraryManager] ERROR: No UID in meta for: %s",
                    assetPath.filename().string().c_str());
                errors++;
                continue;
            }

            // --- TIMESTAMPS: Obtenemos fechas del Asset y del Meta ---
            auto assetTime = fs::last_write_time(assetPath);
            auto metaTime = fs::last_write_time(metaPathStr);

            // Process by type
            switch (type) {
            case AssetType::MODEL_FBX: {
                bool needsImport = false;

                if (meta.libraryPaths.empty()) {
                    // Si no tiene rutas guardadas, hay que importar
                    needsImport = true;
                }
                else {
                    // Comprobamos cada archivo generado en la Library
                    for (const auto& libPath : meta.libraryPaths) {
                        if (!FileExists(libPath)) {
                            needsImport = true; // Falta archivo físico
                            break;
                        }

                        // --- CAMBIO CLAVE: Comprobación de Fecha para Modelos ---
                        auto libTime = fs::last_write_time(libPath);

                        // Si el Asset o el Meta son mas nuevos que la Library -> REIMPORTAR
                        if (assetTime > libTime || metaTime > libTime) {
                            LOG_CONSOLE("[LibraryManager] Change detected in Model/Meta: %s", assetPath.filename().string().c_str());
                            needsImport = true;
                            break;
                        }
                    }
                }

                if (!needsImport) {
                    skipped++;
                    break;
                }

                // --- IMPORTACIÓN FBX ---
                unsigned int importFlags = aiProcess_Triangulate |
                    aiProcess_JoinIdenticalVertices |
                    aiProcess_ValidateDataStructure;

                if (meta.importSettings.generateNormals) importFlags |= aiProcess_GenNormals;
                if (meta.importSettings.flipUVs) importFlags |= aiProcess_FlipUVs;
                if (meta.importSettings.optimizeMeshes) importFlags |= aiProcess_OptimizeMeshes;

                const aiScene* scene = aiImportFile(assetPath.string().c_str(), importFlags);

                if (scene && scene->HasMeshes()) {
                    LOG_DEBUG("Re-Importing FBX: %s", assetPath.filename().string().c_str());

                    meta.libraryPaths.clear();

                    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
                        aiMesh* aiMesh = scene->mMeshes[i];
                        Mesh mesh = MeshImporter::ImportFromAssimp(aiMesh);

                        if (meta.importSettings.importScale != 1.0f) {
                            for (auto& vertex : mesh.vertices) {
                                vertex.position *= meta.importSettings.importScale;
                            }
                        }

                        std::string meshFilename = MeshImporter::GenerateMeshFilename(aiMesh->mName.C_Str());

                        if (MeshImporter::SaveToCustomFormat(mesh, meshFilename)) {
                            std::string fullMeshPath = GetMeshPath(meshFilename);
                            meta.AddLibraryPath(fullMeshPath);
                        }
                    }

                    if (!meta.libraryPaths.empty()) {
                        meta.Save(metaPathStr);
                    }

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
                std::string filename = TextureImporter::GenerateTextureFilename(assetPath.string());
                std::string fullPath = GetTexturePath(filename);

                bool needsImport = false;

                // 1. ¿Existe el archivo en Library?
                if (!FileExists(fullPath)) {
                    needsImport = true;
                }
                else {
                    // 2. --- CAMBIO CLAVE: Comprobación de Fecha para Texturas ---
                    auto libTime = fs::last_write_time(fullPath);

                    // Si la textura original (.png) o su configuración (.meta) son más nuevas -> REIMPORTAR
                    if (assetTime > libTime || metaTime > libTime) {
                        LOG_CONSOLE("[LibraryManager] Change detected in Texture/Meta: %s", assetPath.filename().string().c_str());
                        needsImport = true;
                    }
                }

                if (needsImport) {
                    LOG_DEBUG("Importing texture: %s", assetPath.filename().string().c_str());
                    TextureData texture = TextureImporter::ImportFromFile(assetPath.string());

                    if (texture.IsValid()) {
                        if (TextureImporter::SaveToCustomFormat(texture, filename)) {
                            meta.libraryPath = fullPath;
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
                }
                else {
                    // Already exists and up to date, ensure meta has path
                    if (meta.libraryPath.empty()) {
                        meta.libraryPath = fullPath;
                        meta.Save(metaPathStr);
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

    LOG_CONSOLE("[LibraryManager] Scan complete: %d re-imported/new, %d synchronized, %d errors",
        processed, skipped, errors);
}