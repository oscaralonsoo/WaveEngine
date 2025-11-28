#pragma once

#include <string>
#include <filesystem>

// Centralized manager for Library folder structure
class LibraryManager {
public:
    // Initialize all library directories (finds project root automatically)
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

    // Get base paths
    static std::string GetLibraryRoot();
    static std::string GetAssetsRoot();

private:
    static bool s_initialized;
    static std::string s_projectRoot;  // Root directory of the project (parent of build/)
};