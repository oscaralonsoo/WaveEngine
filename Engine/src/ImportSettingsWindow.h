#pragma once

#include "EditorWindow.h"
#include "MetaFile.h"
#include <string>



class ImportSettingsWindow : public EditorWindow
{
public:
    ImportSettingsWindow();
    ~ImportSettingsWindow();

    void Draw() override;

    // Abrir ventana con settings de un asset específico
    void OpenForAsset(const std::string& assetPath);

    // Check if currently editing an asset
    bool IsEditingAsset() const { return !currentAssetPath.empty(); }

private:
    void DrawModelSettings();
    void DrawTextureSettings();
    void ApplyChanges();
    void ResetToDefaults();
    void CancelChanges();

private:
    std::string currentAssetPath;
    MetaFile currentMeta;
    ImportSettings workingSettings;  // Copia temporal para editar

    bool hasUnsavedChanges;
    bool showApplyButton;
};