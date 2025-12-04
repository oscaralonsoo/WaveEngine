#pragma once

#include "EditorWindow.h"
#include <string>
#include <vector>
#include <filesystem>

struct ImVec2;

namespace fs = std::filesystem;

struct AssetEntry
{
    std::string name;
    std::string path;
    bool isDirectory = false;
    std::string extension;
    unsigned long long uid = 0;
    bool inMemory = false;
    int references = 0;
};

class AssetsWindow : public EditorWindow
{
public:
    AssetsWindow();
    ~AssetsWindow() override;

    void Draw() override;

private:
    void DrawFolderTree(const fs::path& path, const std::string& label);
    void DrawAssetsList();
    void DrawAssetItem(const AssetEntry& asset, std::string& pathPendingToLoad);
    void RefreshAssets();
    void ScanDirectory(const fs::path& directory, std::vector<AssetEntry>& outAssets);

    bool DeleteAsset(const AssetEntry& asset);
    bool DeleteDirectory(const fs::path& dirPath);

    const char* GetAssetIcon(const std::string& extension) const;
    bool IsAssetFile(const std::string& extension) const;
    std::string TruncateFileName(const std::string& name, float maxWidth) const;
    void DrawIconShape(const AssetEntry& asset, const ImVec2& pos, const ImVec2& size);

private:
    std::string assetsRootPath;
    std::string currentPath;
    std::vector<AssetEntry> currentAssets;
    AssetEntry* selectedAsset;
    float iconSize;
    bool showInMemoryOnly;

    bool showDeleteConfirmation;
    AssetEntry assetToDelete;
};