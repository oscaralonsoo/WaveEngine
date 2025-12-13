#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "Log.h"
#include <glad/glad.h>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    textureUID(0),
    originalTextureUID(0),
    texturePath(""),
    originalTexturePath("")
{
}

ComponentMaterial::~ComponentMaterial()
{
    ReleaseCurrentTexture();
}

void ComponentMaterial::Update()
{
}

void ComponentMaterial::OnEditor()
{
}

void ComponentMaterial::ReleaseCurrentTexture()
{
    if (textureUID != 0) {
        Application::GetInstance().resources->ReleaseResource(textureUID);
        textureUID = 0;
    }

    texturePath = "";
}

bool ComponentMaterial::LoadTextureByUID(UID uid)
{
    if (uid == 0) {
        LOG_DEBUG("[ComponentMaterial] Invalid UID");
        return false;
    }

    // Release current texture
    ReleaseCurrentTexture();

    // Request texture resource
    ResourceTexture* texResource = dynamic_cast<ResourceTexture*>(
        Application::GetInstance().resources->RequestResource(uid)
        );

    if (!texResource) {
        LOG_CONSOLE("[ComponentMaterial] ERROR: Failed to load texture with UID: %llu", uid);
        return false;
    }

    if (!texResource->IsLoadedToMemory()) {
        LOG_CONSOLE("[ComponentMaterial] ERROR: Texture not loaded into memory for UID: %llu", uid);
        Application::GetInstance().resources->ReleaseResource(uid);
        return false;
    }

    // Store UID
    textureUID = uid;
    originalTextureUID = uid;

    // Update path for compatibility
    texturePath = texResource->GetAssetFile();
    originalTexturePath = texturePath;

    return true;
}

bool ComponentMaterial::LoadTexture(const std::string& path)
{
    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        LOG_CONSOLE("[ComponentMaterial] ERROR: ModuleResources not available");
        return false;
    }

    UID uid = resources->Find(path.c_str());

    if (uid == 0) {
        // No existe, intentar importar
        uid = resources->ImportFile(path.c_str());

        if (uid == 0) {
            LOG_CONSOLE("[ComponentMaterial] ERROR: Failed to import texture");
            return false;
        }
    }

    return LoadTextureByUID(uid);
}

void ComponentMaterial::CreateCheckerboardTexture()
{
    // Release current texture
    ReleaseCurrentTexture();

    texturePath = "[Checkerboard Pattern]";
    originalTexturePath = "";
}

void ComponentMaterial::Use()
{
    if (textureUID == 0) {
        // No texture, unbind
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    // Get texture resource
    const Resource* resource = Application::GetInstance().resources->GetResource(textureUID);

    if (!resource || !resource->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    const ResourceTexture* texResource = dynamic_cast<const ResourceTexture*>(resource);
    if (texResource) {
        glBindTexture(GL_TEXTURE_2D, texResource->GetGPU_ID());
    }
    else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void ComponentMaterial::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

int ComponentMaterial::GetTextureWidth() const
{
    if (textureUID == 0) return 0;

    const Resource* resource = Application::GetInstance().resources->GetResource(textureUID);
    if (!resource || !resource->IsLoadedToMemory()) return 0;

    const ResourceTexture* texResource = dynamic_cast<const ResourceTexture*>(resource);
    return texResource ? texResource->GetWidth() : 0;
}

int ComponentMaterial::GetTextureHeight() const
{
    if (textureUID == 0) return 0;

    const Resource* resource = Application::GetInstance().resources->GetResource(textureUID);
    if (!resource || !resource->IsLoadedToMemory()) return 0;

    const ResourceTexture* texResource = dynamic_cast<const ResourceTexture*>(resource);
    return texResource ? texResource->GetHeight() : 0;
}

void ComponentMaterial::RestoreOriginalTexture()
{
    if (originalTextureUID != 0) {
        LoadTextureByUID(originalTextureUID);
    }
    else {
        CreateCheckerboardTexture();
    }
}

void ComponentMaterial::Serialize(nlohmann::json& componentObj) const
{
    // Serialize texture UIDs
    if (textureUID != 0) {
        componentObj["textureUID"] = textureUID;
    }
    if (originalTextureUID != 0) {
        componentObj["originalTextureUID"] = originalTextureUID;
    }
}

void ComponentMaterial::Deserialize(const nlohmann::json& componentObj)
{
    // Original texture UID first
    if (componentObj.contains("originalTextureUID")) {
        originalTextureUID = componentObj["originalTextureUID"].get<UID>();
    }

    // Then
    if (componentObj.contains("textureUID")) {
        UID uid = componentObj["textureUID"].get<UID>();
        if (uid != 0) {
            LoadTextureByUID(uid);
        }
    }
}