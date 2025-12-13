#pragma once

#include "EditorWindow.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <imgui.h>

namespace fs = std::filesystem;

// Forward declarations
struct Mesh;

struct AssetEntry
{
    std::string name;
    std::string path;
    std::string extension;
    bool isDirectory;
    bool inMemory;
    unsigned int references;
    unsigned long long uid;

    // Para FBX expandibles
    bool isFBX;
    bool isExpanded;
    std::vector<AssetEntry> subMeshes;  // Mallas contenidas en el FBX

    // Preview/thumbnail
    unsigned int previewTextureID = 0;  // OpenGL texture ID para el preview
    bool previewLoaded = false;         // Si ya se intentó cargar el preview
};

// Tipos de assets para drag & drop
enum class DragDropAssetType
{
    UNKNOWN = 0,
    FBX_MODEL,      // FBX completo (todas las meshes)
    MESH,           // Mesh individual
    TEXTURE,        // Texture (PNG, JPG, DDS, etc)
};

// Payload para drag & drop interno
struct DragDropPayload
{
    std::string assetPath;
    unsigned long long assetUID;
    DragDropAssetType assetType;
};

class AssetsWindow : public EditorWindow
{
public:
    AssetsWindow();
    ~AssetsWindow();

    void Draw() override;

private:
    void RefreshAssets();
    void ScanDirectory(const fs::path& directory, std::vector<AssetEntry>& outAssets);

    void DrawFolderTree(const fs::path& path, const std::string& label);
    void DrawAssetsList();
    void DrawAssetItem(const AssetEntry& asset, std::string& pathPendingToLoad);
    void DrawIconShape(const AssetEntry& entry, const ImVec2& pos, const ImVec2& size);

    const char* GetAssetIcon(const std::string& extension) const;
    bool IsAssetFile(const std::string& extension) const;
    std::string TruncateFileName(const std::string& name, float maxWidth) const;

    bool DeleteAsset(const AssetEntry& asset);
    bool DeleteDirectory(const fs::path& dirPath);

    // Funciones para expandir FBX
    void LoadFBXSubMeshes(AssetEntry& fbxAsset);
    void DrawExpandableAssetItem(AssetEntry& asset, std::string& pathPendingToLoad);

    // Preview/Thumbnail system
    void LoadPreviewForAsset(AssetEntry& asset);
    void UnloadPreviewForAsset(AssetEntry& asset);
    unsigned int RenderMeshToTexture(const Mesh& mesh, int width, int height);
    unsigned int RenderMultipleMeshesToTexture(const std::vector<const Mesh*>& meshes, int width, int height);

    std::string assetsRootPath;
    std::string currentPath;
    std::string sceneRootPath;
    std::vector<AssetEntry> currentAssets;

    AssetEntry* selectedAsset;
    float iconSize;
    bool showInMemoryOnly;

    bool showDeleteConfirmation;
    AssetEntry assetToDelete;

    // Para rastrear qué FBX están expandidos
    std::unordered_set<std::string> expandedFBXPaths;
};