#include "MetaFile.h"
#include "LibraryManager.h"
#include <random>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <windows.h>
#include "Log.h"

// META FILE IMPLEMENTATION
///////////////////////////////

UID MetaFile::GenerateUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<UID> dis;
    return dis(gen);
}

AssetType MetaFile::GetAssetType(const std::string& extension) {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".fbx") return AssetType::MODEL_FBX;
    if (ext == ".png") return AssetType::TEXTURE_PNG;
    if (ext == ".jpg" || ext == ".jpeg") return AssetType::TEXTURE_JPG;
    if (ext == ".dds") return AssetType::TEXTURE_DDS;
    if (ext == ".tga") return AssetType::TEXTURE_TGA;

    return AssetType::UNKNOWN;
}

bool MetaFile::Save(const std::string& metaFilePath) const {
    std::ofstream file(metaFilePath);
    if (!file.is_open()) {
        std::cerr << "[MetaFile] ERROR: Cannot create .meta file: " << metaFilePath << std::endl;
        return false;
    }

    // Convert absolute paths to relative before saving
    std::string relativeOriginalPath = MakeRelativeToProject(originalPath);

    // Write data 
    file << "uid: " << uid << "\n";
    file << "type: " << static_cast<int>(type) << "\n";
    file << "lastModified: " << lastModified << "\n";
    file << "importScale: " << importSettings.importScale << "\n";
    file << "generateNormals: " << (importSettings.generateNormals ? "1" : "0") << "\n";
    file << "flipUVs: " << (importSettings.flipUVs ? "1" : "0") << "\n";
    file << "optimizeMeshes: " << (importSettings.optimizeMeshes ? "1" : "0") << "\n";
    file << "generateMipmaps: " << (importSettings.generateMipmaps ? "1" : "0") << "\n";
    file << "wrapMode: " << importSettings.wrapMode << "\n";

    file.close();
    return true;
}

MetaFile MetaFile::Load(const std::string& metaFilePath) {
    MetaFile meta;

    std::ifstream file(metaFilePath);
    if (!file.is_open()) {
        return meta;
    }

    std::string line;

    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 2);

       
        if (key == "uid") {
            meta.uid = std::stoull(value);
        }
        else if (key == "type") {
            meta.type = static_cast<AssetType>(std::stoi(value));
        }
        else if (key == "originalPath") {
            meta.originalPath = MakeAbsoluteFromProject(value);
        }
        else if (key == "lastModified") {
            meta.lastModified = std::stoll(value);
        }
        else if (key == "importScale") {
            meta.importSettings.importScale = std::stof(value);
        }
        else if (key == "generateNormals") {
            meta.importSettings.generateNormals = (value == "1");
        }
        else if (key == "flipUVs") {
            meta.importSettings.flipUVs = (value == "1");
        }
        else if (key == "optimizeMeshes") {
            meta.importSettings.optimizeMeshes = (value == "1");
        }
        else if (key == "generateMipmaps") {
            meta.importSettings.generateMipmaps = (value == "1");
        }
        else if (key == "wrapMode") {
            meta.importSettings.wrapMode = std::stoi(value);
        }
    }

    file.close();

    return meta;
}

bool MetaFile::NeedsReimport(const std::string& assetPath) const {
    if (!std::filesystem::exists(assetPath)) {
        return false;
    }

    long long currentTimestamp = std::filesystem::last_write_time(assetPath)
        .time_since_epoch().count();

    // Tolerance of a few seconds
    const long long tolerance = 20000000000; 

    long long diff = std::abs(currentTimestamp - lastModified);

    return diff > tolerance;
}

std::string MetaFile::MakeRelativeToProject(const std::string& absolutePath) {
    if (absolutePath.empty()) {
        return "";
    }

    try {
        std::filesystem::path absPath(absolutePath);
        std::filesystem::path projectRoot = LibraryManager::GetAssetsRoot();
        projectRoot = projectRoot.parent_path(); // Assets/ -> Project root

        // If already relative or can't make relative, return as is
        if (absPath.is_relative()) {
            return absolutePath;
        }

        // Make relative to project root
        std::filesystem::path relativePath = std::filesystem::relative(absPath, projectRoot);

        std::string result = relativePath.string();
        std::replace(result.begin(), result.end(), '\\', '/');

        return result;
    }
    catch (...) {
        return absolutePath;
    }
}

std::string MetaFile::MakeAbsoluteFromProject(const std::string& relativePath) {
    if (relativePath.empty()) {
        return "";
    }

    try {
        std::filesystem::path relPath(relativePath);

        // If already absolute, return as is
        if (relPath.is_absolute()) {
            return relativePath;
        }

        // Get project root
        std::filesystem::path projectRoot = LibraryManager::GetAssetsRoot();
        projectRoot = projectRoot.parent_path();

        // Combine project root + relative path
        std::filesystem::path absolutePath = projectRoot / relPath;

        return absolutePath.string();
    }
    catch (...) {
        return relativePath;
    }
}

// METAFILE MANAGER IMPLEMENTATION
///////////////////////////////////////////

void MetaFileManager::Initialize() {
    ScanAssets();
}

void MetaFileManager::ScanAssets() {
    std::string assetsPath = LibraryManager::GetAssetsRoot();

    if (!std::filesystem::exists(assetsPath)) {
        LOG_DEBUG("[MetaFileManager] Assets folder not found: %s", assetsPath.c_str());
        return;
    }

    int metasCreated = 0;
    int metasExisting = 0;

    // Recursively iterate through Assets/
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string assetPath = entry.path().string();
        std::string extension = entry.path().extension().string();

        // Skip .meta files
        if (extension == ".meta") continue;

        AssetType type = MetaFile::GetAssetType(extension);
        if (type == AssetType::UNKNOWN) continue; // Unsupported type

        std::string metaPath = GetMetaPath(assetPath);

        // Only create .meta if it doesn't exist
        if (!std::filesystem::exists(metaPath)) {
            // Create new .meta
            MetaFile meta;
            meta.uid = MetaFile::GenerateUID();
            meta.type = type;
            meta.originalPath = assetPath;
            meta.lastModified = GetFileTimestamp(assetPath);

            if (meta.Save(metaPath)) {
                metasCreated++;
            }
        }
        else {
            metasExisting++;
        }
    }

    LOG_CONSOLE("[MetaFileManager] Scan complete: %d created, %d existing",
        metasCreated, metasExisting);
}

bool MetaFileManager::UpdateMetaIfModified(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    // If no .meta exists, create new one
    if (!std::filesystem::exists(metaPath)) {
        LOG_CONSOLE("[MetaFileManager] No .meta found, creating for: %s", assetPath.c_str());
        GetOrCreateMeta(assetPath);
        return true;
    }

    // Load existing .meta
    MetaFile meta = MetaFile::Load(metaPath);

    // Check if timestamp changed
    long long currentTimestamp = GetFileTimestamp(assetPath);

    if (meta.lastModified != currentTimestamp) {
        LOG_CONSOLE("[MetaFileManager] File modified, updating .meta: %s", assetPath.c_str());

        // Update timestamp
        meta.lastModified = currentTimestamp;

        // Save changes
        if (meta.Save(metaPath)) {
            LOG_CONSOLE("[MetaFileManager] .meta updated successfully");
            return true;
        }
        else {
            LOG_CONSOLE("[MetaFileManager] ERROR: Failed to save .meta");
            return false;
        }
    }

    // No update needed
    return false;
}

MetaFile MetaFileManager::GetOrCreateMeta(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    // Try loading existing .meta
    if (std::filesystem::exists(metaPath)) {
        MetaFile meta = MetaFile::Load(metaPath);

        // If no UID, assign one now
        if (meta.uid == 0) {
            meta.uid = MetaFile::GenerateUID();
            meta.Save(metaPath);
        }

        return meta;
    }

    // Create new .meta
    MetaFile meta;
    meta.uid = MetaFile::GenerateUID();
    meta.type = MetaFile::GetAssetType(std::filesystem::path(assetPath).extension().string());
    meta.originalPath = assetPath;
    meta.lastModified = GetFileTimestamp(assetPath);

    meta.Save(metaPath);

    return meta;
}

bool MetaFileManager::NeedsReimport(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    if (!std::filesystem::exists(metaPath)) {
        return true; // No .meta, needs import
    }

    MetaFile meta = MetaFile::Load(metaPath);
    return meta.NeedsReimport(assetPath);
}

void MetaFileManager::RegenerateLibrary() {
    ScanAssets();
}

MetaFile MetaFileManager::LoadMeta(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    if (std::filesystem::exists(metaPath)) {
        return MetaFile::Load(metaPath);
    }

    return MetaFile();  // Return empty meta if doesn't exist
}

UID MetaFileManager::GetUIDFromAsset(const std::string& assetPath) {
    MetaFile meta = LoadMeta(assetPath);

    // If no UID, generate one and save
    if (meta.uid == 0 && std::filesystem::exists(assetPath)) {
        meta.uid = MetaFile::GenerateUID();
        std::string metaPath = GetMetaPath(assetPath);
        meta.Save(metaPath);
    }

    return meta.uid;
}

std::string MetaFileManager::GetAssetFromUID(UID uid) {
    if (uid == 0) return "";

    std::string assetsPath = LibraryManager::GetAssetsRoot();

    if (!std::filesystem::exists(assetsPath)) {
        return "";
    }

    // Search all .meta files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        std::string extension = entry.path().extension().string();

        // Only .meta files
        if (extension != ".meta") continue;

        MetaFile meta = MetaFile::Load(path);
        if (meta.uid == uid) {
            return meta.originalPath;
        }
    }

    return "";  // Not found
}

long long MetaFileManager::GetFileTimestamp(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return 0;
    }

    return std::filesystem::last_write_time(filePath).time_since_epoch().count();
}

// Private Helpers

std::string MetaFileManager::GetMetaPath(const std::string& assetPath) {
    return assetPath + ".meta";
}