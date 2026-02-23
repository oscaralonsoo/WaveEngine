#include "ComponentMaterial.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "Renderer.h"
#include "Texture.h"
#include "ResourceShader.h"
#include "Log.h"
#include <glad/glad.h>
#include <SDL3/SDL_timer.h>

ComponentMaterial::ComponentMaterial(GameObject* owner)
    : Component(owner, ComponentType::MATERIAL),
    textureUID(0),
    originalTextureUID(0),
    texturePath(""),
    originalTexturePath(""),
    useCheckerboard(false),
    hasMaterialProperties(false),
    lightingMode(1),
    waveSpeed(0.4f),
    waveAmplitude(1.1f),
    waveFrequency(8.2f)
{
}

ComponentMaterial::~ComponentMaterial()
{
    ReleaseCurrentTexture();
    ReleaseCurrentShader();
}

void ComponentMaterial::Update()
{
}

void ComponentMaterial::OnEditor()
{
    // Unified Shader Selector
    std::string currentSelection = "Standard";
    if (shaderUID != 0) {
        ModuleResources* resources = Application::GetInstance().resources.get();
        const Resource* res = resources->GetResourceDirect(shaderUID);
        if (res) {
            currentSelection = res->GetAssetFile();
            size_t lastSlash = currentSelection.find_last_of("/\\");
            if (lastSlash != std::string::npos)
                currentSelection = currentSelection.substr(lastSlash + 1);
        }
        else {
            currentSelection = "Unknown Shader";
        }
    }
    else if (materialType == MaterialType::WATER) {
        currentSelection = "Water";
    }

    ImGui::Text("Shader:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##UnifiedShaderSelector", currentSelection.c_str()))
    {
        bool isStandard = (materialType == MaterialType::STANDARD && shaderUID == 0);
        if (ImGui::Selectable("Standard", isStandard)) {
            materialType = MaterialType::STANDARD;
            ReleaseCurrentShader();
        }
        if (isStandard) ImGui::SetItemDefaultFocus();

        bool isWater = (materialType == MaterialType::WATER && shaderUID == 0);
        if (ImGui::Selectable("Water", isWater)) {
            materialType = MaterialType::WATER;
            ReleaseCurrentShader();
        }
        if (isWater) ImGui::SetItemDefaultFocus();

        ImGui::Separator();
        ImGui::TextDisabled("Custom Shaders");

        ModuleResources* resources = Application::GetInstance().resources.get();
        const auto& allResources = resources->GetAllResources();

        for (const auto& pair : allResources) {
            if (pair.second->GetType() == Resource::SHADER) {
                std::string name = pair.second->GetAssetFile();
                size_t lastSlash = name.find_last_of("/\\");
                if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);

                bool isSelected = (shaderUID == pair.first);
                if (ImGui::Selectable(name.c_str(), isSelected)) {
                    LoadShaderByUID(pair.first);
                    materialType = MaterialType::STANDARD;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (shaderUID != 0 && ImGui::Button("Reload Shader")) {
        LoadShaderByUID(shaderUID);
    }

    if (materialType == MaterialType::WATER)
    {
        ImGui::Separator();
        ImGui::Text("Water Parameters");

        ImGui::DragFloat("Speed", &waveSpeed, 0.1f, 0.0f, 10.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Controls how fast the waves move across the surface.");
            ImGui::EndTooltip();
        }

        ImGui::DragFloat("Amplitude", &waveAmplitude, 0.05f, 0.0f, 5.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Controls the height of the waves.");
            ImGui::EndTooltip();
        }

        ImGui::DragFloat("Frequency", &waveFrequency, 0.1f, 0.0f, 10.0f);
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Controls how many waves appear on the surface (density).");
            ImGui::EndTooltip();
        }
    }

    ImGui::Separator();

    ImGui::Text("Texture:");
    ImGui::SameLine();

    std::string currentTextureName = "None";
    if (IsUsingCheckerboard()) {
        currentTextureName = "[Checkerboard Pattern]";
    }
    else if (HasTexture()) {
        UID currentUID = GetTextureUID();
        ModuleResources* resources = Application::GetInstance().resources.get();
        const Resource* res = resources->GetResourceDirect(currentUID);
        if (res) {
            currentTextureName = std::string(res->GetAssetFile());
            size_t lastSlash = currentTextureName.find_last_of("/\\");
            if (lastSlash != std::string::npos)
                currentTextureName = currentTextureName.substr(lastSlash + 1);
        }
        else {
            currentTextureName = "UID " + std::to_string(currentUID);
        }
    }

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##TextureSelector", currentTextureName.c_str()))
    {
        bool isCheckerboardSelected = IsUsingCheckerboard();
        if (ImGui::Selectable("[Checkerboard Pattern]", isCheckerboardSelected))
            CreateCheckerboardTexture();
        if (isCheckerboardSelected) ImGui::SetItemDefaultFocus();

        ImGui::Separator();

        ModuleResources* resources = Application::GetInstance().resources.get();
        const std::map<UID, Resource*>& allResources = resources->GetAllResources();

        for (const auto& pair : allResources)
        {
            const Resource* res = pair.second;
            if (res->GetType() == Resource::TEXTURE)
            {
                std::string textureName = res->GetAssetFile();
                size_t lastSlash = textureName.find_last_of("/\\");
                if (lastSlash != std::string::npos) textureName = textureName.substr(lastSlash + 1);

                UID texUID = res->GetUID();
                bool isSelected = (!IsUsingCheckerboard() && HasTexture() && GetTextureUID() == texUID);

                std::string displayName = textureName;
                if (res->IsLoadedToMemory()) displayName += " [Loaded]";

                const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                unsigned int gpuID = texRes->GetGPU_ID();

                if (ImGui::Selectable(displayName.c_str(), isSelected))
                    if (LoadTextureByUID(texUID))
                        LOG_DEBUG("Assigned texture '%s' (UID %llu)", textureName.c_str(), texUID);

                if (isSelected) ImGui::SetItemDefaultFocus();

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    if (gpuID != 0) {
                        float tooltipPreviewSize = 128.0f;
                        float width = (float)texRes->GetWidth();
                        float height = (float)texRes->GetHeight();
                        float scale = tooltipPreviewSize / std::max(width, height);
                        ImGui::Image((ImTextureID)(intptr_t)gpuID, ImVec2(width * scale, height * scale));
                        ImGui::Separator();
                    }
                    ImGui::Text("%s", textureName.c_str());
                    ImGui::EndTooltip();
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (HasTexture())
    {
        unsigned int gpuID = 0;
        int texWidth = 0, texHeight = 0;

        if (IsUsingCheckerboard()) {
            Renderer* renderer = Application::GetInstance().renderer.get();
            if (renderer) {
                Texture* defaultTex = renderer->GetDefaultTexture();
                if (defaultTex) {
                    gpuID = defaultTex->GetID();
                    texWidth = defaultTex->GetWidth();
                    texHeight = defaultTex->GetHeight();
                }
            }
        }
        else {
            UID currentUID = GetTextureUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res && res->GetType() == Resource::TEXTURE) {
                const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                gpuID = texRes->GetGPU_ID();
                texWidth = texRes->GetWidth();
                texHeight = texRes->GetHeight();
            }
        }

        if (gpuID != 0)
        {
            ImGui::Text("Texture Preview:");
            float previewMaxSize = 256.0f;
            float scale = previewMaxSize / std::max((float)texWidth, (float)texHeight);
            ImVec2 previewSize(texWidth * scale, texHeight * scale);
            float offsetX = (ImGui::GetContentRegionAvail().x - previewSize.x) * 0.5f;
            if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            ImGui::Image((ImTextureID)(intptr_t)gpuID, previewSize);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        ImGui::Text("Size: %d x %d pixels", GetTextureWidth(), GetTextureHeight());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Actions:");
    ImGui::Spacing();

    if (ImGui::Button("Apply Checkerboard", ImVec2(-1, 0)))
        CreateCheckerboardTexture();

    if (ImGui::Button("Remove Texture", ImVec2(-1, 0)))
        ReleaseCurrentTexture();

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Properties:");
    bool changed = false;

    glm::vec4 diffuse = diffuseColor;
    if (ImGui::ColorEdit4("Diffuse Color", &diffuse.r)) {
        SetDiffuseColor(diffuse);
        changed = true;
    }

    float alpha = opacity;
    if (ImGui::SliderFloat("Opacity", &alpha, 0.0f, 1.0f)) {
        SetOpacity(alpha);
        changed = true;
    }

    ImGui::Separator();
    ImGui::Text("Lighting Mode:");
    const char* modes[] = { "Vertex (Gouraud - Faceted)", "Pixel (Blinn-Phong - Smooth)" };
    if (ImGui::Combo("##LightingMode", &lightingMode, modes, IM_ARRAYSIZE(modes)))
        changed = true;
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

void ComponentMaterial::ReleaseCurrentShader()
{
    if (shaderUID != 0) {
        Application::GetInstance().resources->ReleaseResource(shaderUID);
        shaderUID = 0;
    }
    shaderPath = "";
}

bool ComponentMaterial::LoadShaderByUID(UID uid)
{
    if (uid == 0) return false;

    ReleaseCurrentShader();

    ResourceShader* shRes = dynamic_cast<ResourceShader*>(
        Application::GetInstance().resources->RequestResource(uid)
    );

    if (!shRes) {
        LOG_CONSOLE("[ComponentMaterial] ERROR: Failed to load shader with UID: %llu", uid);
        return false;
    }

    shaderUID = uid;
    shaderPath = shRes->GetAssetFile();
    return true;
}

bool ComponentMaterial::LoadShader(const std::string& path)
{
    ModuleResources* resources = Application::GetInstance().resources.get();
    UID uid = resources->Find(path.c_str());
    if (uid == 0) uid = resources->ImportFile(path.c_str());
    
    return LoadShaderByUID(uid);
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
    
    componentObj["materialType"] = static_cast<int>(materialType);

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
        componentObj["lightingMode"] = lightingMode;
    }

    if (shaderUID != 0) {
        componentObj["shaderUID"] = shaderUID;
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

    if (componentObj.contains("materialType")) {
        materialType = static_cast<MaterialType>(componentObj["materialType"].get<int>());
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
            if (componentObj.contains("lightingMode")) {
                lightingMode = componentObj["lightingMode"].get<int>();
            }
        }
    }
    
    if (componentObj.contains("waveSpeed")) {
        waveSpeed = componentObj["waveSpeed"].get<float>();
    }
    if (componentObj.contains("waveAmplitude")) {
        waveAmplitude = componentObj["waveAmplitude"].get<float>();
    }
    if (componentObj.contains("waveFrequency")) {
        waveFrequency = componentObj["waveFrequency"].get<float>();
    }

    if (componentObj.contains("shaderUID")) {
        UID uid = componentObj["shaderUID"].get<UID>();
        if (uid != 0) LoadShaderByUID(uid);
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