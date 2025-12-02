#include "MetaFile.h"
#include "LibraryManager.h"
#include <random>
#include <iomanip>
#include <iostream>
#include <windows.h>

std::string MetaFile::GenerateGUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);

    std::stringstream ss;
    ss << std::hex << std::setfill('0')
        << std::setw(16) << part1
        << std::setw(16) << part2;

    return ss.str();
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

    // Convertir rutas absolutas a relativas antes de guardar
    std::string relativeOriginalPath = MakeRelativeToProject(originalPath);
    std::string relativeLibraryPath = MakeRelativeToProject(libraryPath);

    // ESCRIBIR LOS DATOS
    file << "guid: " << guid << "\n";
    file << "type: " << static_cast<int>(type) << "\n";
    file << "originalPath: " << relativeOriginalPath << "\n";  
    file << "libraryPath: " << relativeLibraryPath << "\n";    
    file << "lastModified: " << lastModified << "\n";
    file << "importScale: " << importSettings.importScale << "\n";
    file << "generateNormals: " << (importSettings.generateNormals ? "1" : "0") << "\n";  // ← 1/0
    file << "flipUVs: " << (importSettings.flipUVs ? "1" : "0") << "\n";                  // ← 1/0
    file << "optimizeMeshes: " << (importSettings.optimizeMeshes ? "1" : "0") << "\n";    // ← 1/0

    file.close();
    return true;
}

std::string MetaFile::MakeRelativeToProject(const std::string& absolutePath) {
    if (absolutePath.empty()) {
        return "";
    }

    try {
        std::filesystem::path absPath(absolutePath);
        std::filesystem::path projectRoot = LibraryManager::GetAssetsRoot();
        projectRoot = projectRoot.parent_path(); // Assets/ -> Project root

        // Si ya es relativa o no se puede hacer relativa, devolver como esta
        if (absPath.is_relative()) {
            return absolutePath;
        }

        // Hacer relativa respecto al project root
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

        // Si ya es absoluta, devolver como esta
        if (relPath.is_absolute()) {
            return relativePath;
        }

        // Obtener project root
        std::filesystem::path projectRoot = LibraryManager::GetAssetsRoot();
        projectRoot = projectRoot.parent_path();

        // Combinar project root + relative path
        std::filesystem::path absolutePath = projectRoot / relPath;

        return absolutePath.string();
    }
    catch (...) {
        return relativePath;
    }
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

        if (key == "guid") meta.guid = value;
        else if (key == "type") meta.type = static_cast<AssetType>(std::stoi(value));
        else if (key == "originalPath") {
            // Convertir de relativa a absoluta
            meta.originalPath = MakeAbsoluteFromProject(value);
        }
        else if (key == "libraryPath") {
            // Convertir de relativa a absoluta
            meta.libraryPath = MakeAbsoluteFromProject(value);
        }
        else if (key == "lastModified") meta.lastModified = std::stoll(value);
        else if (key == "importScale") meta.importSettings.importScale = std::stof(value);
        else if (key == "generateNormals") meta.importSettings.generateNormals = (value == "1");
        else if (key == "flipUVs") meta.importSettings.flipUVs = (value == "1");
        else if (key == "optimizeMeshes") meta.importSettings.optimizeMeshes = (value == "1");
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

    // Tolerancia de algunos segundos 
    const long long tolerance = 20000000000; // u de 100 nanosegundos

    long long diff = std::abs(currentTimestamp - lastModified);

    return diff > tolerance;
}


///// MetaFileManager Implementation/////

void MetaFileManager::Initialize() {

    ScanAssets();
}

void MetaFileManager::ScanAssets() {
    std::string assetsPath = LibraryManager::GetAssetsRoot();

    if (!std::filesystem::exists(assetsPath)) {
        return;
    }

    int metasCreated = 0;
    int metasUpdated = 0;

    // Iterar recursivamente por Assets/
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string assetPath = entry.path().string();
        std::string extension = entry.path().extension().string();

        // Ignorar archivos .meta
        if (extension == ".meta") continue;

        AssetType type = MetaFile::GetAssetType(extension);
        if (type == AssetType::UNKNOWN) continue; // Tipo no soportado

        std::string metaPath = GetMetaPath(assetPath);

        // Verificar si .meta existe
        if (!std::filesystem::exists(metaPath)) {
            // Crear nuevo .meta
            MetaFile meta;
            meta.guid = MetaFile::GenerateGUID();
            meta.type = type;
            meta.originalPath = assetPath;
            meta.lastModified = GetFileTimestamp(assetPath);
            meta.libraryPath = ""; // Se asignara durante importación

            if (meta.Save(metaPath)) {
                metasCreated++;
            }
        }
        else {
            // Verificar si necesita actualización
            MetaFile meta = MetaFile::Load(metaPath);
            long long currentTimestamp = GetFileTimestamp(assetPath);

            if (meta.lastModified != currentTimestamp) {
                metasUpdated++;
            }
        }
    }

}

MetaFile MetaFileManager::GetOrCreateMeta(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    // Intentar cargar .meta existente
    if (std::filesystem::exists(metaPath)) {
        return MetaFile::Load(metaPath);
    }

    // Crear nuevo .meta
    MetaFile meta;
    meta.guid = MetaFile::GenerateGUID();
    meta.type = MetaFile::GetAssetType(std::filesystem::path(assetPath).extension().string());
    meta.originalPath = assetPath;
    meta.lastModified = GetFileTimestamp(assetPath);

    meta.Save(metaPath);

    return meta;
}

bool MetaFileManager::NeedsReimport(const std::string& assetPath) {
    std::string metaPath = GetMetaPath(assetPath);

    if (!std::filesystem::exists(metaPath)) {
        return true; // No hay .meta, necesita importación
    }

    MetaFile meta = MetaFile::Load(metaPath);
    return meta.NeedsReimport(assetPath);
}

void MetaFileManager::RegenerateLibrary() {
    ScanAssets();
}

// Private Helpers

std::string MetaFileManager::GetMetaPath(const std::string& assetPath) {
    return assetPath + ".meta";
}

long long MetaFileManager::GetFileTimestamp(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return 0;
    }

    return std::filesystem::last_write_time(filePath).time_since_epoch().count();
}