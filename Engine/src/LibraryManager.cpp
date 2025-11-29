#include "LibraryManager.h"
#include <iostream>
#include <windows.h>
#include "MetaFile.h"
#include "TextureImporter.h"

namespace fs = std::filesystem;

bool LibraryManager::s_initialized = false;
fs::path LibraryManager::s_projectRoot;

void LibraryManager::Initialize() {
    if (s_initialized) {
        return;
    }

    std::cout << "[LibraryManager] Initializing library structure..." << std::endl;

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
        std::cout << "[LibraryManager] Project root found at: " << s_projectRoot.string() << std::endl;
    }

    if (!assetsFound) {
        std::cerr << "[LibraryManager] ERROR: Could not find project root (Assets folder not found)" << std::endl;
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
    std::cout << "[LibraryManager] Library structure initialized successfully!" << std::endl;
}

void LibraryManager::EnsureDirectoryExists(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
            std::cout << "[LibraryManager] Created directory: " << path.string() << std::endl;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "[LibraryManager] Error creating directory " << path.string() << ": " << e.what() << std::endl;
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

    std::cout << "\n[LibraryManager] ========================================" << std::endl;
    std::cout << "[LibraryManager] CLEARING LIBRARY FOLDER" << std::endl;
    std::cout << "[LibraryManager] ========================================\n" << std::endl;

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

            std::cout << "[LibraryManager] Deleted " << filesDeleted << " files from Library" << std::endl;

            // Recrear estructura de carpetas
            Initialize();
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "[LibraryManager] ERROR: " << e.what() << std::endl;
    }

    std::cout << "[LibraryManager] Library cleared successfully\n" << std::endl;
}

void LibraryManager::RegenerateFromAssets() {
    std::cout << "\n[LibraryManager] ========================================" << std::endl;
    std::cout << "[LibraryManager] REGENERATING LIBRARY FROM ASSETS" << std::endl;
    std::cout << "[LibraryManager] ========================================\n" << std::endl;

    // Paso 1: Limpiar Library
    ClearLibrary();

    // Paso 2: Escanear Assets y crear/actualizar .meta
    MetaFileManager::ScanAssets();

    // Paso 3: Procesar todos los assets que necesitan reimportación
    fs::path assetsPath = GetAssetsRoot();
    int processed = 0;

    for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        fs::path assetPath = entry.path();
        std::string extension = assetPath.extension().string();

        // Ignorar .meta
        if (extension == ".meta") continue;

        AssetType type = MetaFile::GetAssetType(extension);
        if (type == AssetType::UNKNOWN) continue;

        // Verificar si necesita reimportación
        if (MetaFileManager::NeedsReimport(assetPath.string())) {
            std::cout << "[LibraryManager] Processing: " << assetPath.filename().string() << std::endl;

            // Procesar según el tipo
            switch (type) {
            case AssetType::MODEL_FBX:
                // El FBXLoader ya maneja esto automáticamente
                break;

            case AssetType::TEXTURE_PNG:
            case AssetType::TEXTURE_JPG:
            case AssetType::TEXTURE_DDS: {
                // Importar textura
                TextureData texture = TextureImporter::ImportFromFile(assetPath.string());
                if (texture.IsValid()) {
                    std::string filename = TextureImporter::GenerateTextureFilename(assetPath.string());
                    TextureImporter::SaveToCustomFormat(texture, filename);
                    std::cout << "  ? Texture processed: " << filename << std::endl;
                }
                break;
            }

            default:
                break;
            }

            processed++;
        }
    }

    std::cout << "\n[LibraryManager] Regeneration complete: " << processed << " assets processed" << std::endl;
    std::cout << "[LibraryManager] ========================================\n" << std::endl;
}