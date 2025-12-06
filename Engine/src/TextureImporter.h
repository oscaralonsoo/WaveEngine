#pragma once

#include <string>
#include <glm/glm.hpp>

struct TextureData;

// Header for custom texture format
struct TextureHeader {
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;          // 1=Gray, 3=RGB, 4=RGBA
    unsigned int format = 0;            // GL_RGB, GL_RGBA, etc.
    unsigned int dataSize = 0;          // Pixel data size in bytes
    bool hasAlpha = false;
    bool compressed = false;            // Future use for DDS
};

// Runtime texture data
struct TextureData {
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int channels = 0;
    unsigned char* pixels = nullptr;    // Raw RGBA pixel data

    // Default constructor
    TextureData() = default;

    // Prevent double-free
    TextureData(const TextureData&) = delete;
    TextureData& operator=(const TextureData&) = delete;

    // Move semantics
    TextureData(TextureData&& other) noexcept
        : width(other.width)
        , height(other.height)
        , channels(other.channels)
        , pixels(other.pixels)
    {
        other.pixels = nullptr;
        other.width = 0;
        other.height = 0;
        other.channels = 0;
    }

    TextureData& operator=(TextureData&& other) noexcept {
        if (this != &other) {
            // Clean up existing data
            if (pixels != nullptr) {
                delete[] pixels;
            }

            // Move data
            width = other.width;
            height = other.height;
            channels = other.channels;
            pixels = other.pixels;

            // Nullify source
            other.pixels = nullptr;
            other.width = 0;
            other.height = 0;
            other.channels = 0;
        }
        return *this;
    }

    ~TextureData() {
        if (pixels != nullptr) {
            delete[] pixels;
            pixels = nullptr;
        }
    }

    bool IsValid() const {
        return pixels != nullptr && width > 0 && height > 0;
    }
};

class TextureImporter {
public:
    TextureImporter();
    ~TextureImporter();

    // Load texture via DevIL and convert to TextureData
    static TextureData ImportFromFile(const std::string& filepath);

    // Save TextureData to custom binary format in Library/Textures/
    static bool SaveToCustomFormat(const TextureData& texture, const std::string& filename);

    // Load from custom binary format back to TextureData
    static TextureData LoadFromCustomFormat(const std::string& filename);

    // Generate unique filename based on source path
    static std::string GenerateTextureFilename(const std::string& originalPath);

    // Get OpenGL format enum from channel count
    static unsigned int GetOpenGLFormat(unsigned int channels);

private:
    // Init DevIL (called automatically)
    static void InitDevIL();

    // DevIL initialization state
    static bool s_devilInitialized;
};