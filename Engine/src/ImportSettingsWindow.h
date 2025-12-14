#pragma once

#include "EditorWindow.h"
#include "MetaFile.h"
#include <string>

// Forward declarations
class GameObject;

class ImportSettingsWindow : public EditorWindow
{
public:
    ImportSettingsWindow();
    ~ImportSettingsWindow();

    void Draw() override;
    void OpenForAsset(const std::string& assetPath);
    bool IsEditingAsset() const { return !currentAssetPath.empty(); }

private:
    void DrawModelSettings();
    void DrawTextureSettings();
    void ApplyChanges();
    void ResetToDefaults();
    void CancelChanges();

    // Helper methods for reloading textures in scene
    void ReloadTextureInScene(UID textureUID);
    void ReloadTextureRecursive(GameObject* obj, UID textureUID, int& count);

private:
    std::string currentAssetPath;
    MetaFile currentMeta;
    ImportSettings workingSettings;
    bool hasUnsavedChanges;
    bool showApplyButton;
};