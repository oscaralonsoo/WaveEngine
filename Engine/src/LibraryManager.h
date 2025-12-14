#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class LibraryManager {
public:
    static void Initialize();
    static void EnsureDirectoryExists(const fs::path& path);
    static bool IsInitialized();

    // UID-based paths
    static std::string GetMeshPathFromUID(unsigned long long uid);
    static std::string GetMaterialPathFromUID(unsigned long long uid);
    static std::string GetTexturePathFromUID(unsigned long long uid);
    static std::string GetModelPathFromUID(unsigned long long uid);
    static std::string GetAnimationPathFromUID(unsigned long long uid);

    // Legacy compatibility
    static std::string GetMeshPath(const std::string& filename);
    static std::string GetTexturePath(const std::string& filename);
    static std::string GetModelPath(const std::string& filename);

    static bool FileExists(const fs::path& path);

    // Base paths
    static std::string GetLibraryRoot();
    static std::string GetAssetsRoot();

    // Library management
    static void ClearLibrary();
    static void RegenerateFromAssets();

    // ? NUEVO: Reimportar un solo asset
    static bool ReimportAsset(const std::string& assetPath);

private:
    static bool s_initialized;
    static fs::path s_projectRoot;
};