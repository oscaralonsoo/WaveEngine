#include "Texture.h"
#include <iostream>
#include <windows.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <fstream>
#include "Log.h"
#include "TextureImporter.h"
#include "LibraryManager.h"

#define CHECKERS_WIDTH 64
#define CHECKERS_HEIGHT 64

Texture::Texture() : textureID(0), width(0), height(0), nrChannels(0)
{
}

Texture::~Texture()
{
    if (textureID != 0)
        glDeleteTextures(1, &textureID);
}

void Texture::CreateCheckerboard()
{
    LOG_DEBUG("Creating checkerboard pattern texture");
    // Checkerboard pattern
    static GLubyte checkerImage[CHECKERS_HEIGHT][CHECKERS_WIDTH][4];

    for (int i = 0; i < CHECKERS_HEIGHT; i++) {
        for (int j = 0; j < CHECKERS_WIDTH; j++) {
            int c = ((((i & 0x8) == 0) ^ ((j & 0x8) == 0))) * 255;
            checkerImage[i][j][0] = (GLubyte)c;
            checkerImage[i][j][1] = (GLubyte)c;
            checkerImage[i][j][2] = (GLubyte)c;
            checkerImage[i][j][3] = (GLubyte)255;
        }
    }

    // Generate OpenGL texture
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHECKERS_WIDTH, CHECKERS_HEIGHT,
        0, GL_RGBA, GL_UNSIGNED_BYTE, checkerImage);

    glBindTexture(GL_TEXTURE_2D, 0);

    width = CHECKERS_WIDTH;
    height = CHECKERS_HEIGHT;
    nrChannels = 4;

    LOG_DEBUG("Checkerboard texture created - Size: %dx%d, ID: %d", width, height, textureID);
    LOG_CONSOLE("Default checkerboard texture ready");
}

void Texture::InitDevIL()
{
    static bool initialized = false;
    if (!initialized)
    {
        LOG_DEBUG("Initializing DevIL library");

        ilInit();
        iluInit();
        initialized = true;

        ILint devilVersion = ilGetInteger(IL_VERSION_NUM);
        int devilMajor = devilVersion / 100;
        int devilMinor = (devilVersion / 10) % 10;
        int devilPatch = devilVersion % 10;
        LOG_DEBUG("DevIL initialized successfully - Version: %d.%d.%d", devilMajor, devilMinor, devilPatch);
        LOG_CONSOLE("DevIL library initialized - Version: %d.%d.%d", devilMajor, devilMinor, devilPatch);
    }
}

// Normalize path separators
std::string NormalizePath(const std::string& path)
{
    std::string normalized = path;
    for (char& c : normalized)
    {
        if (c == '\\')
            c = '/';
    }
    return normalized;
}

// Check if path is absolute
bool IsAbsolutePath(const std::string& path)
{
    // Windows: Drive letter (C:, D:, etc.) or UNC path (\\)
    if (path.length() >= 2)
    {
        if (path[1] == ':' || (path[0] == '\\' && path[1] == '\\'))
            return true;
    }
    // Unix: Root (/)
    if (!path.empty() && path[0] == '/')
        return true;

    return false;
}

// Check if file exists
bool FileExists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}