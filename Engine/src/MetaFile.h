#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

typedef unsigned long long UID;

enum class AssetType {
    UNKNOWN = 0,
    MODEL_FBX = 1,
    TEXTURE_PNG = 2,
    TEXTURE_JPG = 3,
    TEXTURE_DDS = 4,
    TEXTURE_TGA = 5
};

struct ImportSettings {
    // Mesh/Model settings
    float importScale = 1.0f;
    bool generateNormals = true;
    bool flipUVs = true;
    bool optimizeMeshes = true;

    // Texture settings
    bool generateMipmaps = true;
    int wrapMode = 0;  // 0=Repeat, 1=Clamp
};

struct MetaFile {
    UID uid = 0;                                // Unique ID of the resource
    AssetType type = AssetType::UNKNOWN;
    std::string originalPath;                   // Path to original asset file

    long long lastModified = 0;                 // Timestamp of last modification
    ImportSettings importSettings;

    static UID GenerateUID();
    static AssetType GetAssetType(const std::string& extension);

    bool Save(const std::string& metaFilePath) const;
    static MetaFile Load(const std::string& metaFilePath);

    bool NeedsReimport(const std::string& assetPath) const;

    // Helpers for relative paths
    static std::string MakeRelativeToProject(const std::string& absolutePath);
    static std::string MakeAbsoluteFromProject(const std::string& relativePath);
};

// Manager for .meta files
class MetaFileManager {
public:
    static void Initialize();
    static void ScanAssets();
    static MetaFile GetOrCreateMeta(const std::string& assetPath);
    static bool NeedsReimport(const std::string& assetPath);
    static void RegenerateLibrary();
    static bool UpdateMetaIfModified(const std::string& assetPath);

    // Useful queries
    static MetaFile LoadMeta(const std::string& assetPath);
    static UID GetUIDFromAsset(const std::string& assetPath);
    static std::string GetAssetFromUID(UID uid);
    static long long GetFileTimestamp(const std::string& filePath);

private:
    static std::string GetMetaPath(const std::string& assetPath);
};