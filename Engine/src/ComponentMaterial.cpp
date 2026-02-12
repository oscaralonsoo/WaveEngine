#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "Renderer.h"
#include "Texture.h"
#include "Log.h"
#include <glad/glad.h>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    textureUID(0),
    originalTextureUID(0),
    texturePath(""),
    originalTexturePath(""),
    useCheckerboard(false),
    hasMaterialProperties(false)  
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
    useCheckerboard = false;
}

bool ComponentMaterial::LoadTextureByUID(UID uid)
{
    if (uid == 0) {
        LOG_DEBUG("[ComponentMaterial] Invalid UID");
        return false;
    }

    ReleaseCurrentTexture();

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

    textureUID = uid;
    originalTextureUID = uid;
    texturePath = texResource->GetAssetFile();
    originalTexturePath = texturePath;
    useCheckerboard = false;

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
    ReleaseCurrentTexture();

    useCheckerboard = true;
    texturePath = "[Checkerboard Pattern]";
    originalTexturePath = "";

    LOG_DEBUG("[ComponentMaterial] Checkerboard texture activated");
}

void ComponentMaterial::Use()
{
    if (useCheckerboard) {
        Renderer* renderer = Application::GetInstance().renderer.get();
        if (renderer && renderer->GetDefaultTexture()) {
            renderer->GetDefaultTexture()->Bind();
        }
        else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        return;
    }

    if (textureUID == 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

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
    if (useCheckerboard) {
        return 64;
    }

    if (textureUID == 0) return 0;

    const Resource* resource = Application::GetInstance().resources->GetResource(textureUID);
    if (!resource || !resource->IsLoadedToMemory()) return 0;

    const ResourceTexture* texResource = dynamic_cast<const ResourceTexture*>(resource);
    return texResource ? texResource->GetWidth() : 0;
}

int ComponentMaterial::GetTextureHeight() const
{
    if (useCheckerboard) {
        return 64;
    }

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
    if (textureUID != 0) {
        componentObj["textureUID"] = textureUID;
    }
    if (originalTextureUID != 0) {
        componentObj["originalTextureUID"] = originalTextureUID;
    }
    componentObj["useCheckerboard"] = useCheckerboard;

    componentObj["hasMaterialProperties"] = hasMaterialProperties;

    if (hasMaterialProperties) {
        componentObj["diffuseColor"] = { diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a };
        componentObj["specularColor"] = { specularColor.r, specularColor.g, specularColor.b, specularColor.a };
        componentObj["ambientColor"] = { ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a };
        componentObj["emissiveColor"] = { emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveColor.a };
        componentObj["shininess"] = shininess;
        componentObj["opacity"] = opacity;
        componentObj["metallic"] = metallic;
        componentObj["roughness"] = roughness;
    }
    if (shaderUID != 0) componentObj["shaderUID"] = shaderUID;

    // uniforms
    if (!uniformOverrides.empty())
    {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& kv : uniformOverrides)
        {
            nlohmann::json u;
            u["name"] = kv.first;
            u["type"] = (int)kv.second.type;
            u["v4"] = { kv.second.v4.x, kv.second.v4.y, kv.second.v4.z, kv.second.v4.w };
            u["i"] = kv.second.i;
            u["b"] = kv.second.b;
            arr.push_back(u);
        }
        componentObj["uniforms"] = arr;
    }
}

void ComponentMaterial::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("originalTextureUID")) {
        originalTextureUID = componentObj["originalTextureUID"].get<UID>();
    }

    if (componentObj.contains("useCheckerboard")) {
        useCheckerboard = componentObj["useCheckerboard"].get<bool>();
    }

    if (componentObj.contains("textureUID")) {
        UID uid = componentObj["textureUID"].get<UID>();
        if (uid != 0) {
            LoadTextureByUID(uid);
        }
    }
    else if (useCheckerboard) {
        CreateCheckerboardTexture();
    }

    if (componentObj.contains("hasMaterialProperties")) {
        hasMaterialProperties = componentObj["hasMaterialProperties"].get<bool>();

        if (hasMaterialProperties) {
            if (componentObj.contains("diffuseColor")) {
                auto color = componentObj["diffuseColor"];
                diffuseColor = glm::vec4(color[0], color[1], color[2], color[3]);
            }
            if (componentObj.contains("specularColor")) {
                auto color = componentObj["specularColor"];
                specularColor = glm::vec4(color[0], color[1], color[2], color[3]);
            }
            if (componentObj.contains("ambientColor")) {
                auto color = componentObj["ambientColor"];
                ambientColor = glm::vec4(color[0], color[1], color[2], color[3]);
            }
            if (componentObj.contains("emissiveColor")) {
                auto color = componentObj["emissiveColor"];
                emissiveColor = glm::vec4(color[0], color[1], color[2], color[3]);
            }
            if (componentObj.contains("shininess")) {
                shininess = componentObj["shininess"].get<float>();
            }
            if (componentObj.contains("opacity")) {
                opacity = componentObj["opacity"].get<float>();
            }
            if (componentObj.contains("metallic")) {
                metallic = componentObj["metallic"].get<float>();
            }
            if (componentObj.contains("roughness")) {
                roughness = componentObj["roughness"].get<float>();
            }
        }
    }
    if (componentObj.contains("shaderUID"))
        shaderUID = componentObj["shaderUID"].get<UID>();

    uniformOverrides.clear();
    if (componentObj.contains("uniforms"))
    {
        for (auto& u : componentObj["uniforms"])
        {
            UniformValue v;
            v.type = (ResourceShader::UniformType)u["type"].get<int>();
            auto a = u["v4"];
            v.v4 = glm::vec4(a[0], a[1], a[2], a[3]);
            v.i = u.value("i", 0);
            v.b = u.value("b", false);
            uniformOverrides[u["name"].get<std::string>()] = v;
        }
    }
}

void ComponentMaterial::ReloadTexture()
{
    if (textureUID == 0 || useCheckerboard) {
        return;
    }

    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        return;
    }

    LOG_DEBUG("[ComponentMaterial] Reloading texture UID: %llu", textureUID);

    // Release current reference
    resources->ReleaseResource(textureUID);

    // Request it again - this will load the reimported version
    ResourceTexture* texResource = dynamic_cast<ResourceTexture*>(
        resources->RequestResource(textureUID)
        );

    if (!texResource) {
        LOG_DEBUG("[ComponentMaterial] ERROR: Failed to reload texture");
        textureUID = 0;
        texturePath = "";
        return;
    }

    if (!texResource->IsLoadedToMemory()) {
        LOG_DEBUG("[ComponentMaterial] ERROR: Reloaded texture not in memory");
        resources->ReleaseResource(textureUID);
        textureUID = 0;
        texturePath = "";
        return;
    }

    // Texture is now reloaded with new settings
    texturePath = texResource->GetAssetFile();
    LOG_DEBUG("[ComponentMaterial] Texture reloaded successfully (GPU_ID: %u)",
        texResource->GetGPU_ID());
}
bool ComponentMaterial::HasUniform(const std::string& name) const
{
    return uniformOverrides.find(name) != uniformOverrides.end();
}

ComponentMaterial::UniformValue* ComponentMaterial::GetUniform(const std::string& name)
{
    auto it = uniformOverrides.find(name);
    return (it != uniformOverrides.end()) ? &it->second : nullptr;
}

const ComponentMaterial::UniformValue* ComponentMaterial::GetUniform(const std::string& name) const
{
    auto it = uniformOverrides.find(name);
    return (it != uniformOverrides.end()) ? &it->second : nullptr;
}

void ComponentMaterial::SetUniform(const std::string& name, const UniformValue& v)
{
    uniformOverrides[name] = v;
}

void ComponentMaterial::RemoveUniform(const std::string& name)
{
    uniformOverrides.erase(name);
}
