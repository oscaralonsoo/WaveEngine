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
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Shader Type Selector
        const char* materialTypes[] = { "Standard", "Water" };
        int currentType = static_cast<int>(materialType);
        
        if (ImGui::Combo("Shader Type", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes)))
        {
            materialType = static_cast<MaterialType>(currentType);
        }

        if (materialType == MaterialType::WATER)
        {
            ImGui::Separator();
            ImGui::Text("Water Parameters");
            ImGui::DragFloat("Speed", &waveSpeed, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Amplitude", &waveAmplitude, 0.05f, 0.0f, 5.0f);
            ImGui::DragFloat("Frequency", &waveFrequency, 0.1f, 0.0f, 10.0f);
        }

        ImGui::Separator();

        // Texture handling
        ImGui::Text("Texture:");
        ImGui::SameLine();

        std::string currentTextureName = "None";

        if (IsUsingCheckerboard()) {
            currentTextureName = "[Checkerboard Pattern]";
        }
        else if (HasTexture())
        {
            UID currentUID = GetTextureUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res)
            {
                currentTextureName = std::string(res->GetAssetFile());
                size_t lastSlash = currentTextureName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    currentTextureName = currentTextureName.substr(lastSlash + 1);
            }
            else
            {
                currentTextureName = "UID " + std::to_string(currentUID);
            }
        }

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##TextureSelector", currentTextureName.c_str()))
        {
            bool isCheckerboardSelected = IsUsingCheckerboard();
            if (ImGui::Selectable("[Checkerboard Pattern]", isCheckerboardSelected))
            {
                CreateCheckerboardTexture();
            }

            if (isCheckerboardSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

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
                    bool isSelected = (!IsUsingCheckerboard() &&
                        HasTexture() &&
                        GetTextureUID() == texUID);

                    std::string displayName = textureName;
                    if (res->IsLoadedToMemory())
                    {
                        displayName += " [Loaded]";
                    }

                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    unsigned int gpuID = texRes->GetGPU_ID();

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        if (LoadTextureByUID(texUID))
                        {
                            LOG_DEBUG("Assigned texture '%s' (UID %llu)", textureName.c_str(), texUID);
                        }
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();

                        if (gpuID != 0)
                        {
                            float tooltipPreviewSize = 128.0f;
                            float width = (float)texRes->GetWidth();
                            float height = (float)texRes->GetHeight();
                            float scale = tooltipPreviewSize / std::max(width, height);
                            ImVec2 tooltipSize(width * scale, height * scale);

                            ImGui::Image((ImTextureID)(intptr_t)gpuID, tooltipSize);
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
            int texWidth = 0;
            int texHeight = 0;

            if (IsUsingCheckerboard())
            {
                Renderer* renderer = Application::GetInstance().renderer.get();
                if (renderer)
                {
                    Texture* defaultTex = renderer->GetDefaultTexture();
                    if (defaultTex)
                    {
                        gpuID = defaultTex->GetID();
                        texWidth = defaultTex->GetWidth();
                        texHeight = defaultTex->GetHeight();
                    }
                }
            }
            else
            {
                UID currentUID = GetTextureUID();
                ModuleResources* resources = Application::GetInstance().resources.get();
                const Resource* res = resources->GetResourceDirect(currentUID);

                if (res && res->GetType() == Resource::TEXTURE)
                {
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
                float width = (float)texWidth;
                float height = (float)texHeight;

                float scale = previewMaxSize / std::max(width, height);
                ImVec2 previewSize(width * scale, height * scale);

                float windowWidth = ImGui::GetContentRegionAvail().x;
                float offsetX = (windowWidth - previewSize.x) * 0.5f;
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
        {
            CreateCheckerboardTexture();
        }

        if (ImGui::Button("Remove Texture", ImVec2(-1, 0))) {
            ReleaseCurrentTexture();
        }

        ImGui::Spacing();
        
        ImGui::Separator();
        
        // Material Properties
        ImGui::Text("Properties:");
        
        bool changed = false;
        
        glm::vec4 diffuse = diffuseColor;
        if (ImGui::ColorEdit4("Diffuse Color", &diffuse.r)) {
            SetDiffuseColor(diffuse);
            changed = true;
        }
    }
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