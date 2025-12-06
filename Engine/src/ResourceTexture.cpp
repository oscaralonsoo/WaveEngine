#include "ResourceTexture.h"
#include "TextureImporter.h"
#include "Log.h"
#include <glad/glad.h>

ResourceTexture::ResourceTexture(UID uid)
    : Resource(uid, Resource::TEXTURE) {
}

ResourceTexture::~ResourceTexture() {
    UnloadFromMemory();
}

bool ResourceTexture::LoadInMemory() {
    if (loadedInMemory) {
        LOG_DEBUG("[ResourceTexture] Already loaded in memory");
        return true;
    }

    if (libraryFile.empty()) {
        LOG_DEBUG("[ResourceTexture] ERROR: No library file specified");
        return false;
    }

    LOG_DEBUG("[ResourceTexture] Loading texture from: %s", libraryFile.c_str());

    // Extract just the filename (no full path)
    std::string filename = libraryFile;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // Load via TextureImporter
    TextureData textureData = TextureImporter::LoadFromCustomFormat(filename);

    if (!textureData.IsValid()) {
        LOG_DEBUG("[ResourceTexture] ERROR: Failed to load texture data");
        return false;
    }

    // Create OpenGL texture
    glGenTextures(1, &gpu_id);
    glBindTexture(GL_TEXTURE_2D, gpu_id);

    // Setup texture params
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload to GPU
    GLenum format = (textureData.channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format,
        textureData.width, textureData.height,
        0, format, GL_UNSIGNED_BYTE, textureData.pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Store info
    width = textureData.width;
    height = textureData.height;
    depth = textureData.channels;
    bytes = textureData.width * textureData.height * textureData.channels;
    format = (textureData.channels == 4) ? RGBA : RGB;

    loadedInMemory = true;

    LOG_DEBUG("[ResourceTexture] Loaded successfully:");
    LOG_DEBUG("  GPU ID: %u", gpu_id);
    LOG_DEBUG("  Size: %ux%u", width, height);
    LOG_DEBUG("  Channels: %u", depth);

    return true;
}

void ResourceTexture::UnloadFromMemory() {
    if (!loadedInMemory) {
        return;
    }

    if (gpu_id != 0) {
        LOG_DEBUG("[ResourceTexture] Unloading texture GPU ID: %u", gpu_id);
        glDeleteTextures(1, &gpu_id);
        gpu_id = 0;
    }

    width = 0;
    height = 0;
    depth = 0;
    bytes = 0;
    format = UNKNOWN;

    loadedInMemory = false;
}