#pragma once

#include <string>
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

    // Texture settings (para futuro)
    bool generateMipmaps = true;
    int wrapMode = 0;  // 0=Repeat, 1=Clamp
};

struct MetaFile {
    UID uid = 0;                    // UID único del recurso
    std::string guid;               // GUID (mantener por compatibilidad)
    AssetType type = AssetType::UNKNOWN;
    std::string originalPath;
    std::string libraryPath;
    long long lastModified = 0;
    ImportSettings importSettings;

    static std::string GenerateGUID();
    static UID GenerateUID();
    static AssetType GetAssetType(const std::string& extension);

    bool Save(const std::string& metaFilePath) const;
    static MetaFile Load(const std::string& metaFilePath);

    bool NeedsReimport(const std::string& assetPath) const;

    // Helpers para rutas relativas
    static std::string MakeRelativeToProject(const std::string& absolutePath);
    static std::string MakeAbsoluteFromProject(const std::string& relativePath);
};

// Manager para archivos .meta
class MetaFileManager {
public:
    static void Initialize();
    static void ScanAssets();
    static MetaFile GetOrCreateMeta(const std::string& assetPath);
    static bool NeedsReimport(const std::string& assetPath);
    static void RegenerateLibrary();

    // Queries útiles
    static MetaFile LoadMeta(const std::string& assetPath);
    static UID GetUIDFromAsset(const std::string& assetPath);
    static std::string GetAssetFromUID(UID uid);
    static long long GetFileTimestamp(const std::string& filePath);

private:
    static std::string GetMetaPath(const std::string& assetPath);
};