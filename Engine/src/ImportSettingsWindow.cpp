#include "ImportSettingsWindow.h"
#include "ModuleResources.h"
#include "MetaFile.h"
#include "LibraryManager.h"
#include "Application.h"
#include "FileSystem.h"
#include "Log.h"
#include <imgui.h>
#include <filesystem>

ImportSettingsWindow::ImportSettingsWindow()
    : EditorWindow("Import Settings"),
    hasUnsavedChanges(false),
    showApplyButton(false)
{
}

ImportSettingsWindow::~ImportSettingsWindow()
{
}

void ImportSettingsWindow::OpenForAsset(const std::string& assetPath)
{
    if (!std::filesystem::exists(assetPath))
    {
        LOG_CONSOLE("[ImportSettings] Asset not found: %s", assetPath.c_str());
        return;
    }

    currentAssetPath = assetPath;

    // Load or create meta
    std::string metaPath = assetPath + ".meta";
    if (std::filesystem::exists(metaPath))
    {
        currentMeta = MetaFile::Load(metaPath);
    }
    else
    {
        currentMeta = MetaFileManager::GetOrCreateMeta(assetPath);
    }

    // Copy settings to working copy
    workingSettings = currentMeta.importSettings;

    hasUnsavedChanges = false;
    showApplyButton = false;
    isOpen = true;

    LOG_CONSOLE("[ImportSettings] Opened settings for: %s", assetPath.c_str());
}

void ImportSettingsWindow::Draw()
{
    if (!isOpen || currentAssetPath.empty())
        return;

    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Import Settings", &isOpen))
    {
        isHovered = ImGui::IsWindowHovered();

        // Asset info header
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Asset:");
        ImGui::SameLine();

        std::string filename = std::filesystem::path(currentAssetPath).filename().string();
        ImGui::TextWrapped("%s", filename.c_str());

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "UID: %llu", currentMeta.uid);

        ImGui::Separator();

        // Settings based on asset type
        if (currentMeta.type == AssetType::MODEL_FBX)
        {
            DrawModelSettings();
        }
        else if (currentMeta.type == AssetType::TEXTURE_PNG ||
            currentMeta.type == AssetType::TEXTURE_JPG ||
            currentMeta.type == AssetType::TEXTURE_DDS ||
            currentMeta.type == AssetType::TEXTURE_TGA)
        {
            DrawTextureSettings();
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                "No import settings available for this asset type.");
        }

        ImGui::Separator();

        // Action buttons
        ImGui::Spacing();

        if (hasUnsavedChanges)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Unsaved changes");
        }

        ImGui::Spacing();

        bool canApply = hasUnsavedChanges;

        if (!canApply)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
        }

        if (ImGui::Button("Apply", ImVec2(100, 0)))
        {
            if (canApply)
            {
                ApplyChanges();
            }
        }

        if (!canApply)
        {
            ImGui::PopStyleVar();
        }

        if (ImGui::IsItemHovered() && canApply)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Save settings to .meta file");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset to Defaults", ImVec2(150, 0)))
        {
            ResetToDefaults();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(80, 0)))
        {
            CancelChanges();
        }
    }
    ImGui::End();
}

void ImportSettingsWindow::DrawModelSettings()
{
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Model Import Settings");
    ImGui::Spacing();

    bool changed = false;

    // Import Scale
    ImGui::Text("Import Scale");
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::DragFloat("##importScale", &workingSettings.importScale, 0.01f, 0.001f, 100.0f, "%.3f"))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Scale factor applied on import");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Axis Configuration
    ImGui::Text("Axis Configuration");
    ImGui::Spacing();

    const char* upAxisOptions[] = { "Y-Up", "Z-Up" };
    if (ImGui::Combo("Up Axis", &workingSettings.upAxis, upAxisOptions, IM_ARRAYSIZE(upAxisOptions)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Y-Up: Standard for most 3D apps (Blender, Maya)\nZ-Up: Standard for CAD apps");
        ImGui::EndTooltip();
    }

    const char* frontAxisOptions[] = { "Z-Forward", "Y-Forward", "X-Forward" };
    if (ImGui::Combo("Front Axis", &workingSettings.frontAxis, frontAxisOptions, IM_ARRAYSIZE(frontAxisOptions)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Which axis points forward in the model");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Post-processing options
    ImGui::Text("Post-Processing");
    ImGui::Spacing();

    if (ImGui::Checkbox("Generate Normals", &workingSettings.generateNormals))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Generate smooth normals if missing");
        ImGui::EndTooltip();
    }

    if (ImGui::Checkbox("Flip UVs", &workingSettings.flipUVs))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Flip texture coordinates vertically");
        ImGui::EndTooltip();
    }

    if (ImGui::Checkbox("Optimize Meshes", &workingSettings.optimizeMeshes))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Merge meshes and optimize vertex cache");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Info box
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Note:");
    ImGui::TextWrapped("Changes will be saved to .meta file and applied on next import.");

    if (changed && !hasUnsavedChanges)
    {
        hasUnsavedChanges = true;
    }
}

void ImportSettingsWindow::DrawTextureSettings()
{
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Texture Import Settings");
    ImGui::Spacing();

    bool changed = false;

    // Filtering
    ImGui::Text("Filtering");
    ImGui::Spacing();

    const char* filterModes[] = { "Point (Nearest)", "Bilinear", "Trilinear" };
    if (ImGui::Combo("Filter Mode", &workingSettings.filterMode, filterModes, IM_ARRAYSIZE(filterModes)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Point: Sharp, pixelated (retro games)\nBilinear: Smooth, slightly blurry\nTrilinear: Smooth with mipmaps (best quality)");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();

    // Mipmaps
    if (ImGui::Checkbox("Generate Mipmaps", &workingSettings.generateMipmaps))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Generate mipmap levels for better quality at distance\nRequired for Trilinear filtering");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Wrapping
    ImGui::Text("Wrapping Mode");
    ImGui::Spacing();

    const char* wrapModes[] = { "Repeat", "Clamp to Edge", "Mirrored Repeat", "Clamp to Border" };
    if (ImGui::Combo("Wrap Mode", &workingSettings.wrapMode, wrapModes, IM_ARRAYSIZE(wrapModes)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("How texture coordinates outside [0,1] are handled:\n"
            "Repeat: Tile the texture (default)\n"
            "Clamp: Stretch edge pixels\n"
            "Mirrored: Tile with mirror effect\n"
            "Border: Use border color");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Flip options
    ImGui::Text("Flip Options");
    ImGui::Spacing();

    if (ImGui::Checkbox("Flip Vertically (Y)", &workingSettings.flipUVs))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Flip texture vertically on load\nUseful for correcting upside-down textures");
        ImGui::EndTooltip();
    }

    if (ImGui::Checkbox("Flip Horizontally (X)", &workingSettings.flipHorizontal))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Flip texture horizontally on load\nUseful for mirrored textures");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Compression
    ImGui::Text("Compression");
    ImGui::Spacing();

    const char* compressionOptions[] = { "None", "DXT1 (RGB)", "DXT5 (RGBA)", "BC7 (High Quality)" };
    if (ImGui::Combo("Compression", &workingSettings.compressionFormat, compressionOptions, IM_ARRAYSIZE(compressionOptions)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Texture compression format:\n"
            "None: No compression (high quality, large size)\n"
            "DXT1: 6:1 compression, no alpha\n"
            "DXT5: 4:1 compression, with alpha\n"
            "BC7: Best quality, slower compression");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Max Size
    ImGui::Text("Max Texture Size");
    ImGui::Spacing();

    const char* textureSizes[] = { "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192" };
    if (ImGui::Combo("Max Size", &workingSettings.maxTextureSize, textureSizes, IM_ARRAYSIZE(textureSizes)))
    {
        changed = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Maximum texture dimension\nTextures larger than this will be downscaled");
        ImGui::EndTooltip();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Info box
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Note:");
    ImGui::TextWrapped("Changes will be saved to .meta file and applied on next import.");

    if (changed && !hasUnsavedChanges)
    {
        hasUnsavedChanges = true;
    }
}

void ImportSettingsWindow::ApplyChanges()
{
    if (!hasUnsavedChanges)
        return;

    currentMeta.importSettings = workingSettings;

    std::string metaPath = currentAssetPath + ".meta";
    if (currentMeta.Save(metaPath)) {
        LOG_CONSOLE("[ImportSettings] Settings saved to .meta: %s", currentAssetPath.c_str());

        if (LibraryManager::ReimportAsset(currentAssetPath)) {
            LOG_CONSOLE("[ImportSettings] Asset reimported with new settings!");

            ModuleResources* resources = Application::GetInstance().resources.get();
            if (resources && currentMeta.uid != 0) {

                if (currentMeta.type == AssetType::TEXTURE_PNG ||
                    currentMeta.type == AssetType::TEXTURE_JPG ||
                    currentMeta.type == AssetType::TEXTURE_DDS ||
                    currentMeta.type == AssetType::TEXTURE_TGA)
                {
                    Resource* resource = const_cast<Resource*>(resources->GetResource(currentMeta.uid));
                    if (resource && resource->IsLoadedToMemory()) {
                        LOG_CONSOLE("[ImportSettings] Force unloading texture (refs: %d)",
                            resource->GetReferenceCount());

                        resource->ForceUnload();

                        LOG_CONSOLE("[ImportSettings] Texture unloaded - will reload on next use");
                    }
                }
                else if (currentMeta.type == AssetType::MODEL_FBX)
                {
                    // Para FBX, descargar todas las meshes
                    int unloadedCount = 0;
                    for (int i = 0; i < 100; i++) {
                        unsigned long long meshUID = currentMeta.uid + i;
                        Resource* resource = const_cast<Resource*>(resources->GetResource(meshUID));

                        if (!resource) break;

                        if (resource->IsLoadedToMemory()) {
                            resource->ForceUnload();
                            unloadedCount++;
                        }
                    }
                    LOG_CONSOLE("[ImportSettings] %d meshes unloaded - will reload on next use", unloadedCount);
                }
            }
        }
        else {
            LOG_CONSOLE("[ImportSettings] ERROR: Failed to reimport asset");
        }

        hasUnsavedChanges = false;
    }
    else {
        LOG_CONSOLE("[ImportSettings] ERROR: Failed to save settings");
    }
}
void ImportSettingsWindow::ResetToDefaults()
{
    // Reset to default values
    workingSettings.importScale = 1.0f;
    workingSettings.generateNormals = true;
    workingSettings.flipUVs = true;
    workingSettings.optimizeMeshes = true;
    workingSettings.generateMipmaps = true;
    workingSettings.wrapMode = 0;  // Repeat

    // Nuevos valores
    workingSettings.upAxis = 0;  // Y-Up
    workingSettings.frontAxis = 0;  // Z-Forward
    workingSettings.filterMode = 2;  // Trilinear
    workingSettings.flipHorizontal = false;
    workingSettings.compressionFormat = 0;  // None
    workingSettings.maxTextureSize = 6;  // 2048 (index en el combo)

    hasUnsavedChanges = true;

    LOG_CONSOLE("[ImportSettings] Reset to defaults");
}

void ImportSettingsWindow::CancelChanges()
{
    if (hasUnsavedChanges)
    {
        // Restore original settings
        workingSettings = currentMeta.importSettings;
        hasUnsavedChanges = false;
    }

    // Close window
    isOpen = false;
    currentAssetPath.clear();

    LOG_CONSOLE("[ImportSettings] Cancelled changes");
}