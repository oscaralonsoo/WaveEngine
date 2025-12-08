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
        ilInit();
        iluInit();
        s_devilInitialized = true;
    }
}

// Load texture using DevIL
TextureData TextureImporter::ImportFromFile(const std::string& filepath) {
    InitDevIL();

    TextureData texture;

    // Check file exists
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
        LOG_DEBUG("[TextureImporter] ERROR: DevIL failed to load image: %s", iluErrorString(error));
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Convert to RGBA for consistency
    if (!ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE)) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to convert to RGBA format");
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Get image properties
    texture.width = ilGetInteger(IL_IMAGE_WIDTH);
    texture.height = ilGetInteger(IL_IMAGE_HEIGHT);
    texture.channels = 4; // Converted to RGBA above

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

    // Sanity check (max 100MB)
    if (dataSize == 0 || dataSize > 100000000) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid data size: %zu bytes", dataSize);
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Allocate memory
    try {
        texture.pixels = new unsigned char[dataSize];
    }
    catch (const std::bad_alloc&) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to allocate %zu bytes", dataSize);
        ilDeleteImages(1, &imageID);
        return texture;
    }

    // Copy pixel data
    memcpy(texture.pixels, data, dataSize);

    // Cleanup
    ilDeleteImages(1, &imageID);

    return texture;
}

// Save to custom binary format
bool TextureImporter::SaveToCustomFormat(const TextureData& texture, const std::string& filename) {
    std::string fullPath = LibraryManager::GetTexturePath(filename);

    // Validate texture data before use
    if (!texture.IsValid()) {
        LOG_DEBUG("[TextureImporter] ERROR: Invalid texture data");
        return false;
    }

    if (texture.pixels == nullptr) {
        LOG_DEBUG("[TextureImporter] ERROR: texture.pixels is nullptr");
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

    // Write pixel data
    file.write(reinterpret_cast<const char*>(texture.pixels), header.dataSize);

    if (!file.good()) {
        LOG_DEBUG("[TextureImporter] ERROR: Failed to write pixel data");
        file.close();
        return false;
    }

    file.close();

    return true;
}

// Load from custom binary format
TextureData TextureImporter::LoadFromCustomFormat(const std::string& filename) {
    std::string fullPath = LibraryManager::GetTexturePath(filename);

    TextureData texture;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        LOG_DEBUG("[TextureImporter] ERROR: Could not open file for reading: %s", fullPath.c_str());
        return texture;
    }

    // Read header
    TextureHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(TextureHeader));

    // Validate header
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

    // Safety check (max 100MB)
    if (header.dataSize > 100000000) {
        LOG_DEBUG("[TextureImporter] ERROR: Data size too large: %u bytes", header.dataSize);
        file.close();
        return texture;
    }

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

    return texture;
}

std::string TextureImporter::GenerateTextureFilename(const std::string& originalPath) {
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

    // Convert to lowercase
    std::transform(canonicalPath.begin(), canonicalPath.end(),
        canonicalPath.begin(), ::tolower);

    std::replace(canonicalPath.begin(), canonicalPath.end(), '\\', '/');

    // Extract filename without extension
    std::string basename = path.stem().string();

    if (basename.empty()) {
        basename = "unnamed_texture";
    }

    // Sanitize
    std::replace(basename.begin(), basename.end(), ' ', '_');
    basename.erase(
        std::remove_if(basename.begin(), basename.end(),
            [](char c) { return !std::isalnum(c) && c != '_'; }),
        basename.end()
    );
    std::transform(basename.begin(), basename.end(), basename.begin(), ::tolower);

    // Generate hash
    std::hash<std::string> hasher;
    size_t hashValue = hasher(canonicalPath);

    // Use .texture extension for our custom format
    std::stringstream ss;
    ss << basename << "_" << std::hex << hashValue << ".texture";

    return ss.str();
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