#pragma once

#include <string>
#include <filesystem>

// Centralized manager for Library folder structure
class LibraryManager {
public:
    // Root directories
    static constexpr const char* LIBRARY_ROOT = "Library/";
    static constexpr const char* ASSETS_ROOT = "Assets/";

    // Library subdirectories
    static constexpr const char* MESHES_DIR = "Library/Meshes/";
    static constexpr const char* MATERIALS_DIR = "Library/Materials/";
    static constexpr const char* TEXTURES_DIR = "Library/Textures/";
    static constexpr const char* MODELS_DIR = "Library/Models/";
    static constexpr const char* ANIMATIONS_DIR = "Library/Animations/";

    // Initialize all library directories
    static void Initialize();

    // Ensure specific directory exists
    static void EnsureDirectoryExists(const std::string& path);

    // Check if library is initialized
    static bool IsInitialized();

    // Get full path for a library file
    static std::string GetMeshPath(const std::string& filename);
    static std::string GetMaterialPath(const std::string& filename);
    static std::string GetTexturePath(const std::string& filename);
    static std::string GetModelPath(const std::string& filename);
    static std::string GetAnimationPath(const std::string& filename);

    // Check if a file exists in library
    static bool FileExists(const std::string& path);

private:
    static bool s_initialized;
};