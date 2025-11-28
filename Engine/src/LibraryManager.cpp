#include "LibraryManager.h"
#include <iostream>
#include <windows.h>

bool LibraryManager::s_initialized = false;
std::string LibraryManager::s_projectRoot = "";

void LibraryManager::Initialize() {
    if (s_initialized) {
        return;
    }

    std::cout << "[LibraryManager] Initializing library structure..." << std::endl;

    // Get executable directory
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string execPath(buffer);

    size_t pos = execPath.find_last_of("\\/");
    std::string currentDir = execPath.substr(0, pos);

    // Go up 2 levels from executable (build/Debug/ -> build/ -> ProjectRoot/)
    // First level up (Debug/ -> build/)
    pos = currentDir.find_last_of("\\/");
    if (pos != std::string::npos) {
        currentDir = currentDir.substr(0, pos);

        // Second level up (build/ -> ProjectRoot/)
        pos = currentDir.find_last_of("\\/");
        if (pos != std::string::npos) {
            currentDir = currentDir.substr(0, pos);
        }
    }

    // Verify Assets folder exists at this level
    std::string testPath = currentDir + "\\Assets";
    DWORD attribs = GetFileAttributesA(testPath.c_str());
    bool assetsFound = (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));

    if (assetsFound) {
        s_projectRoot = currentDir;
        std::cout << "[LibraryManager] Project root found at: " << s_projectRoot << std::endl;
    }

    if (!assetsFound)
    {
        std::cerr << "[LibraryManager] ERROR: Could not find project root (Assets folder not found)" << std::endl;
        return;
    }

    // Create Library root at project level
    std::string libraryRoot = s_projectRoot + "\\Library";
    EnsureDirectoryExists(libraryRoot);

    // Create all Library subdirectories
    EnsureDirectoryExists(libraryRoot + "\\Meshes");
    EnsureDirectoryExists(libraryRoot + "\\Materials");
    EnsureDirectoryExists(libraryRoot + "\\Textures");
    EnsureDirectoryExists(libraryRoot + "\\Models");
    EnsureDirectoryExists(libraryRoot + "\\Animations");

    s_initialized = true;
    std::cout << "[LibraryManager] Library structure initialized successfully!" << std::endl;
}

void LibraryManager::EnsureDirectoryExists(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
            std::cout << "[LibraryManager] Created directory: " << path << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[LibraryManager] Error creating directory " << path << ": " << e.what() << std::endl;
    }
}

bool LibraryManager::IsInitialized() {
    return s_initialized;
}

std::string LibraryManager::GetLibraryRoot() {
    return s_projectRoot + "\\Library\\";
}

std::string LibraryManager::GetAssetsRoot() {
    return s_projectRoot + "\\Assets\\";
}

std::string LibraryManager::GetMeshPath(const std::string& filename) {
    return GetLibraryRoot() + "Meshes\\" + filename;
}

std::string LibraryManager::GetMaterialPath(const std::string& filename) {
    return GetLibraryRoot() + "Materials\\" + filename;
}

std::string LibraryManager::GetTexturePath(const std::string& filename) {
    return GetLibraryRoot() + "Textures\\" + filename;
}

std::string LibraryManager::GetModelPath(const std::string& filename) {
    return GetLibraryRoot() + "Models\\" + filename;
}

std::string LibraryManager::GetAnimationPath(const std::string& filename) {
    return GetLibraryRoot() + "Animations\\" + filename;
}

bool LibraryManager::FileExists(const std::string& path) {
    return std::filesystem::exists(path);
}