#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Centralized manager for Library folder structure
class LibraryManager {
public:
    // Init all library directories (finds project root automatically)
    static void Initialize();

    // Ensure directory exists
    static void EnsureDirectoryExists(const fs::path& path);

    // Check if library is initialized
    static bool IsInitialized();

    // Get full path for library file using UID
    static std::string GetMeshPathFromUID(unsigned long long uid);
    static std::string GetMaterialPathFromUID(unsigned long long uid);
    static std::string GetTexturePathFromUID(unsigned long long uid);
    static std::string GetModelPathFromUID(unsigned long long uid);
    static std::string GetAnimationPathFromUID(unsigned long long uid);

    // Legacy compatibility methods (deprecated - use UID versions instead)
    static std::string GetMeshPath(const std::string& filename);
    static std::string GetTexturePath(const std::string& filename);
    static std::string GetModelPath(const std::string& filename);

    // Check if file exists in library
    static bool FileExists(const fs::path& path);

    // Get base paths
    static std::string GetLibraryRoot();
    static std::string GetAssetsRoot();

    // Clear entire Library folder
    static void ClearLibrary();

    // Regenerate Library from Assets using .meta files
    static void RegenerateFromAssets();

private:
    static bool s_initialized;
    static fs::path s_projectRoot;  // Project root directory (parent of build/)
};