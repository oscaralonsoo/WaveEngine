#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Texture.h"
#include <iostream>
#include "Log.h"

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    texture(nullptr),
    texturePath(""),
    originalTexturePath(""),
    hasOriginalTexture(false)
{
    CreateCheckerboardTexture();
}

ComponentMaterial::~ComponentMaterial()
{

}

void ComponentMaterial::Update()
{

}

void ComponentMaterial::OnEditor()
{

}

bool ComponentMaterial::LoadTexture(const std::string& path)
{
    auto newTexture = std::make_unique<Texture>();

    if (newTexture->LoadFromLibraryOrFile(path))
    {
        texture = std::move(newTexture);
        texturePath = path;
        originalTexturePath = path;
        hasOriginalTexture = true;

        LOG_CONSOLE("Texture loaded: %s", path.c_str());


        return true;
    }

    LOG_CONSOLE("Failed to load texture: %s", path.c_str());
    return false;
}

void ComponentMaterial::CreateCheckerboardTexture()
{
    texture = std::make_unique<Texture>();
    texture->CreateCheckerboard();
    texturePath = "[Checkerboard Pattern]";
}

void ComponentMaterial::Use()
{
    if (texture)
    {
        texture->Bind();
    }
}

void ComponentMaterial::Unbind()
{
    if (texture)
    {
        texture->Unbind();
    }
}

int ComponentMaterial::GetTextureWidth() const
{
    if (texture)
    {
        return texture->GetWidth();
    }
    return 0;
}

int ComponentMaterial::GetTextureHeight() const
{
    if (texture)
    {
        return texture->GetHeight();
    }
    return 0;
}

void ComponentMaterial::RestoreOriginalTexture()
{
    if (hasOriginalTexture && !originalTexturePath.empty())
    {
        auto newTexture = std::make_unique<Texture>();

        if (newTexture->LoadFromLibraryOrFile(originalTexturePath))
        {
            texture = std::move(newTexture);
            texturePath = originalTexturePath;
            LOG_CONSOLE("Original texture restored: %s", originalTexturePath.c_str());
        }
        else
        {
            LOG_CONSOLE("Failed to restore original texture");
            CreateCheckerboardTexture();
        }
    }
    else
    {
        LOG_CONSOLE("No original texture available to restore");
    }
}