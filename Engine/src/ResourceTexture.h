#pragma once

#include "ModuleResources.h"

class ResourceTexture : public Resource {
public:
    enum Format {
        COLOR_INDEX,
        RGB,
        RGBA,
        BGR,
        BGRA,
        LUMINANCE,
        UNKNOWN
    };

    ResourceTexture(UID uid);
    virtual ~ResourceTexture();

    // Virtual method implementation
    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // Texture-specific getters
    unsigned int GetWidth() const { return width; }
    unsigned int GetHeight() const { return height; }
    unsigned int GetDepth() const { return depth; }
    unsigned int GetGPU_ID() const { return gpu_id; }
    Format GetFormat() const { return format; }


public:
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int depth = 0;
    unsigned int mips = 0;
    unsigned int bytes = 0;
    unsigned int gpu_id = 0;  // OpenGL texture ID
    Format format = UNKNOWN;
};