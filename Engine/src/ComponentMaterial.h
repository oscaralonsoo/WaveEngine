#pragma once

#include "Component.h"
#include "ModuleResources.h"  
#include <string>

class ComponentMaterial : public Component {
public:
    ComponentMaterial(GameObject* owner);
    ~ComponentMaterial();

    void Update() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    bool LoadTextureByUID(UID uid);

    bool LoadTexture(const std::string& path);

    void CreateCheckerboardTexture();
    void RestoreOriginalTexture();

    void Use();
    void Unbind();

    bool HasTexture() const { return textureUID != 0; }
    bool HasOriginalTexture() const { return originalTextureUID != 0; }

    UID GetTextureUID() const { return textureUID; }
    UID GetOriginalTextureUID() const { return originalTextureUID; }

    //  por compatibilidad (deprecated)
    const std::string& GetTexturePath() const { return texturePath; }
    const std::string& GetOriginalTexturePath() const { return originalTexturePath; }

    int GetTextureWidth() const;
    int GetTextureHeight() const;

private:
    void ReleaseCurrentTexture();

private:
    UID textureUID;
    UID originalTextureUID;

    //  paths por compatibilidad (deprecated)
    std::string texturePath;
    std::string originalTexturePath;
};