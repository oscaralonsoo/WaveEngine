#include "ResourceTexture.h"
#include "TextureImporter.h"
#include "Log.h"
#include <glad/glad.h>
#include "MetaFile.h"

ResourceTexture::ResourceTexture(UID uid)
    : Resource(uid, Resource::TEXTURE) {
}

ResourceTexture::~ResourceTexture() {
    UnloadFromMemory();
}

bool ResourceTexture::LoadInMemory() {
    if (loadedInMemory) {
        return true;
    }

    if (libraryFile.empty()) {
        LOG_DEBUG("[ResourceTexture] ERROR: No library file specified");
        return false;
    }

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

    // ========== LEER .META Y APLICAR SETTINGS ==========
    MetaFile meta = MetaFileManager::LoadMeta(assetsFile);

    if (meta.uid != 0) {
        // Aplicar Wrap Mode desde .meta
        GLenum wrapMode = GL_REPEAT;
        switch (meta.importSettings.wrapMode) {
        case 0: wrapMode = GL_REPEAT; break;
        case 1: wrapMode = GL_CLAMP_TO_EDGE; break;
        case 2: wrapMode = GL_MIRRORED_REPEAT; break;
        case 3: wrapMode = GL_CLAMP_TO_BORDER; break;
        default: wrapMode = GL_REPEAT; break;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    }
    else {
        // Valores por defecto si no hay .meta
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Filtros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload to GPU
    GLenum format = (textureData.channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format,
        textureData.width, textureData.height,
        0, format, GL_UNSIGNED_BYTE, textureData.pixels);

    // Generar mipmaps
    if (meta.uid != 0 && meta.importSettings.generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        glGenerateMipmap(GL_TEXTURE_2D); // Por defecto siempre los generamos
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // Store info
    width = textureData.width;
    height = textureData.height;
    depth = textureData.channels;
    bytes = textureData.width * textureData.height * textureData.channels;
    format = (textureData.channels == 4) ? RGBA : RGB;

    loadedInMemory = true;

    return true;
}

void ResourceTexture::UnloadFromMemory() {
    if (!loadedInMemory) {
        return;
    }

    if (gpu_id != 0) {
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