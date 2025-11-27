#include "LibraryManager.h"
#include <iostream>

bool LibraryManager::s_initialized = false;

void LibraryManager::Initialize() {
    if (s_initialized) {
        return;
    }

    std::cout << "[LibraryManager] Initializing library structure..." << std::endl;

    // Create Assets directory
    EnsureDirectoryExists(ASSETS_ROOT);

    // Create Library root
    EnsureDirectoryExists(LIBRARY_ROOT);

    // Create all Library subdirectories
    EnsureDirectoryExists(MESHES_DIR);
    EnsureDirectoryExists(MATERIALS_DIR);
    EnsureDirectoryExists(TEXTURES_DIR);
    EnsureDirectoryExists(MODELS_DIR);
    EnsureDirectoryExists(ANIMATIONS_DIR);

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

std::string LibraryManager::GetMeshPath(const std::string& filename) {
    return std::string(MESHES_DIR) + filename;
}

std::string LibraryManager::GetMaterialPath(const std::string& filename) {
    return std::string(MATERIALS_DIR) + filename;
}

std::string LibraryManager::GetTexturePath(const std::string& filename) {
    return std::string(TEXTURES_DIR) + filename;
}

std::string LibraryManager::GetModelPath(const std::string& filename) {
    return std::string(MODELS_DIR) + filename;
}

std::string LibraryManager::GetAnimationPath(const std::string& filename) {
    return std::string(ANIMATIONS_DIR) + filename;
}

bool LibraryManager::FileExists(const std::string& path) {
    return std::filesystem::exists(path);
}