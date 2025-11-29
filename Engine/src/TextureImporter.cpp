#include "TextureImporter.h"
#include "LibraryManager.h"
#include "Log.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>

bool TextureImporter::s_devilInitialized = false;

TextureImporter::TextureImporter() {}
TextureImporter::~TextureImporter() {}

void TextureImporter::InitDevIL() {
    if (!s_devilInitialized) {
        LOG_DEBUG("[TextureImporter] Initializing DevIL library");

        ilInit();
        iluInit();

        s_devilInitialized = true;

        ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
        int devilMajor = devilVersion / 100;
        int devilMinor = (devilVersion / 10) % 10;
        int devilPatch = devilVersion % 10;

        LOG_DEBUG("[TextureImporter] DevIL initialized - Version: %d.%d.%d",
            devilMajor, devilMinor, devilPatch);
    }
}

// IMPORT: Load texture using DevIL
TextureData TextureImporter::ImportFromFile(const std::string& filepath) {
    InitDevIL();

    TextureData texture;

    LOG_DEBUG("[TextureImporter] Importing texture: %s", filepath.c_str());

    // Verify file exists
    if (!std::filesystem::exists(filepath)) {
        LOG_DEBUG("[TextureImporter] ERROR: File does not exist: %s", filepath.c_str());
        return texture;
    }

    // Generate DevIL image ID
    ILuint imageID;
    ilGenImages(1, &imageID);
    ilBindImage(imageID);

    // Load image
    if (!ilLoadImage(filepath.c_str())) {
        ILenum error = ilGetError();
        LOG_DEBUG("[TextureImporter] ERROR: DevIL failed to load image");
        LOG_DEBUG("[TextureImporter] DevIL Error: %s", iluErrorString(error));
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Convert to RGBA format for consistency
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to convert to RGBA format");
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Extract image properties
    texture.width = ilGetInteger(IL_IMAGE_WIDTH);
    texture.height = ilGetInteger(IL_IMAGE_HEIGHT);
    texture.channels = 4; // We converted to RGBA above

    // Validate dimensions
    if (texture.width == 0 || texture.height == 0) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid image dimensions");
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Get pixel data
    ILubyte* data = ilGetData();
    if (!data) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to get image data");
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Calculate data size
    size_t dataSize = static_cast<size_t>(texture.width) * static_cast<size_t>(texture.height) * 4;

    // Validate data size is reasonable (max 100MB for safety)
    if (dataSize == 0 || dataSize > 100000000) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid data size: %zu bytes", dataSize);
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Allocate memory safely
    try {
        texture.pixels = new unsigned char[dataSize];
    }
    catch (const std::bad_alloc&) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to allocate %zu bytes", dataSize);
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Copy pixel data to our structure
    memcpy(texture.pixels, data, dataSize);

    LOG_DEBUG("[TextureImporter] Texture imported successfully:");
    LOG_DEBUG("  Width: %d", texture.width);
    LOG_DEBUG("  Height: %d", texture.height);
    LOG_DEBUG("  Channels: %d", texture.channels);
    LOG_DEBUG("  Data size: %.2f KB", dataSize / 1024.0f);

    // Clean up DevIL resources
    ilDeleteImages(1, &imageID);

    return texture;
}

// SAVE: Save to custom binary format
bool TextureImporter::SaveToCustomFormat(const TextureData& texture, const std::string& filename) {
    std::string fullPath = LibraryManager::GetTexturePath(filename);

    LOG_DEBUG("[TextureImporter] Saving texture to: %s", fullPath.c_str());

    // Validate texture data BEFORE using it
    if (!texture.IsValid()) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid texture data (IsValid failed)");
        return false;
    }

    if (texture.pixels == nullptr) {
        LOG_DEBUG("[TextureImporter] ERROR: texture.pixels is nullptr!");
        return false;
    }

    if (texture.width == 0 || texture.height == 0) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid dimensions: %ux%u", texture.width, texture.height);
        return false;
    }

    // Calculate expected data size
    size_t expectedSize = static_cast<size_t>(texture.width) * static_cast<size_t>(texture.height) * 4;
    if (expectedSize == 0 || expectedSize > 100000000) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid data size: %zu bytes", expectedSize);
        return false;
    }

    LOG_DEBUG("[TextureImporter] Texture validation passed:");
    LOG_DEBUG("  Width: %u", texture.width);
    LOG_DEBUG("  Height: %u", texture.height);
    LOG_DEBUG("  Channels: %u", texture.channels);
    LOG_DEBUG("  Pixels pointer: %p", texture.pixels);
    LOG_DEBUG("  Expected size: %zu bytes", expectedSize);

    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("[TextureImporter] ERROR: Could not open file for writing: %s", fullPath.c_str());
        return false;
    }

    // Build header
    TextureHeader header;
    header.width = texture.width;
    header.height = texture.height;
    header.channels = 4; 
    header.format = GetOpenGLFormat(4);
    header.dataSize = static_cast<unsigned int>(expectedSize);
    header.hasAlpha = (texture.channels == 4);
    header.compressed = false;

    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(TextureHeader));

    if (!file.good()) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to write header");
        file.close();
        return false;
    }

    LOG_DEBUG("[TextureImporter] Header written, writing pixel data...");

    // Write pixel data - aqui ha petado como 67 veces
    file.write(reinterpret_cast<const char*>(texture.pixels), header.dataSize);

    if (!file.good()) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to write pixel data");
        file.close();
        return false;
    }

    file.close();

    LOG_DEBUG("[TextureImporter] Texture saved successfully!");
    LOG_DEBUG("  File size: %.2f KB", (sizeof(TextureHeader) + header.dataSize) / 1024.0f);

    return true;
}

// LOAD: Load from custom binary format
TextureData TextureImporter::LoadFromCustomFormat(const std::string& filename) {
    std::string fullPath = LibraryManager::GetTexturePath(filename);

    LOG_DEBUG("[TextureImporter] Loading texture from: %s", fullPath.c_str());

    TextureData texture;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("[TextureImporter] ERROR: Could not open file for reading: %s", fullPath.c_str());
        return texture;
    }

    // Read header
    TextureHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TextureHeader));

    // Validate header data
    if (!file.good()) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to read header");
        file.close();
        return texture;
    }

    if (header.width == 0 || header.height == 0 || header.dataSize == 0) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid header - width=%u, height=%u, dataSize=%u",
            header.width, header.height, header.dataSize);
        file.close();
        return texture;
    }

    // Safety check: max 100MB
    if (header.dataSize > 100000000) {
        LOG_DEBUG("[TextureImporter] ERROR: Data size too large: %u bytes", header.dataSize);
        file.close();
        return texture;
    }

    LOG_DEBUG("[TextureImporter] Header info:");
    LOG_DEBUG("  Width: %u", header.width);
    LOG_DEBUG("  Height: %u", header.height);
    LOG_DEBUG("  Channels: %u", header.channels);
    LOG_DEBUG("  Has Alpha: %s", header.hasAlpha ? "Yes" : "No");
    LOG_DEBUG("  Data size: %.2f KB", header.dataSize / 1024.0f);

    // Allocate and read pixel data
    texture.width = header.width;
    texture.height = header.height;
    texture.channels = header.channels;

    try {
        texture.pixels = new unsigned char[header.dataSize];
    }
    catch (const std::bad_alloc&) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to allocate %u bytes", header.dataSize);
        file.close();
        return texture;
    }

    file.read(reinterpret_cast<char*>(texture.pixels), header.dataSize);

    if (!file.good()) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to read pixel data");
        delete[] texture.pixels;
        texture.pixels = nullptr;
        file.close();
        return texture;
    }

    file.close();

    LOG_DEBUG("[TextureImporter] Texture loaded successfully!");

    return texture;
}

std::string TextureImporter::GenerateTextureFilename(const std::string& originalPath) {
    // Normalizar el path ANTES de generar el hash
    std::filesystem::path path(originalPath);

    std::string canonicalPath;
    try {
        if (std::filesystem::exists(path)) {
            canonicalPath = std::filesystem::canonical(path).string();
        }
        else {
            canonicalPath = std::filesystem::absolute(path).string();
        }
    }
    catch (...) {
        canonicalPath = originalPath;
    }

    // Convertir a lowercase para evitar diferencias Windows (C: vs c:)
    std::transform(canonicalPath.begin(), canonicalPath.end(),
        canonicalPath.begin(), ::tolower);

    // Reemplazar backslashes por forward slashes 
    std::replace(canonicalPath.begin(), canonicalPath.end(), '\\', '/');

    LOG_DEBUG("[TextureImporter] Normalized path: %s", canonicalPath.c_str());
    LOG_DEBUG("[TextureImporter] Original path: %s", originalPath.c_str());

    // Extract filename without extension (del path original, no del normalizado)
    std::string basename = path.stem().string();

    if (basename.empty()) {
        basename = "unnamed_texture";
    }

    // Sanitize: remove spaces and special characters
    std::replace(basename.begin(), basename.end(), ' ', '_');

    basename.erase(
        std::remove_if(basename.begin(), basename.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
        basename.end()
    );

    // Convert to lowercase for consistency
    std::transform(basename.begin(), basename.end(), basename.begin(), ::tolower);

    // Generate hash from CANONICAL path for uniqueness
    std::hash<std::string> hasher;
    size_t hashValue = hasher(canonicalPath);  //  Usa el path normalizado

    std::stringstream ss;
    ss << basename << "_" << std::hex << hashValue << ".texture";

    std::string result = ss.str();
    LOG_DEBUG("[TextureImporter] Generated filename: %s", result.c_str());

    return result;
}
unsigned int TextureImporter::GetOpenGLFormat(unsigned int channels) {
    switch (channels) {
    case 1: return 0x1903; // GL_RED
    case 2: return 0x8227; // GL_RG
    case 3: return 0x1907; // GL_RGB
    case 4: return 0x1908; // GL_RGBA
    default: return 0x1908; // GL_RGBA by default
    }
}