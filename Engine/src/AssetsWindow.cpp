#include "AssetsWindow.h"
#include <imgui.h>
#include "Application.h"
#include "LibraryManager.h"
#include "MetaFile.h"
#include "ModuleResources.h"
#include "Log.h"

AssetsWindow::AssetsWindow()
    : EditorWindow("Assets"), selectedAsset(nullptr), iconSize(64.0f),
    showInMemoryOnly(false), showDeleteConfirmation(false)
{
    if (!LibraryManager::IsInitialized()) {
        LibraryManager::Initialize();
    }

    assetsRootPath = LibraryManager::GetAssetsRoot();
    currentPath = assetsRootPath;
}

AssetsWindow::~AssetsWindow()
{
}

std::string AssetsWindow::TruncateFileName(const std::string& name, float maxWidth) const
{
    ImVec2 textSize = ImGui::CalcTextSize(name.c_str());

    if (textSize.x <= maxWidth)
        return name;

    std::string truncated = name;
    std::string suffix = "...";
    ImVec2 suffixSize = ImGui::CalcTextSize(suffix.c_str());

    while (truncated.length() > 3)
    {
        truncated = truncated.substr(0, truncated.length() - 1);
        ImVec2 currentSize = ImGui::CalcTextSize((truncated + suffix).c_str());

        if (currentSize.x <= maxWidth)
            return truncated + suffix;
    }

    return suffix;
}

void AssetsWindow::DrawIconShape(const AssetEntry& asset, const ImVec2& pos, const ImVec2& size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec4 buttonColor;

    if (asset.isDirectory) {
        buttonColor = asset.inMemory ?
            ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
            ImVec4(0.8f, 0.7f, 0.3f, 1.0f);
    }
    else {
        buttonColor = asset.inMemory ?
            ImVec4(0.3f, 0.8f, 0.3f, 1.0f) :
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }

    ImU32 color = ImGui::ColorConvertFloat4ToU32(buttonColor);
    ImU32 outlineColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

    ImVec2 center = ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    float padding = size.x * 0.15f;

    if (asset.isDirectory)
    {
        float w = size.x - padding * 2;
        float h = size.y - padding * 2;
        ImVec2 topLeft(pos.x + padding, pos.y + padding + h * 0.2f);
        ImVec2 bottomRight(pos.x + size.x - padding, pos.y + size.y - padding);

        ImVec2 tabStart(topLeft.x, topLeft.y);
        ImVec2 tabEnd(topLeft.x + w * 0.4f, topLeft.y);
        ImVec2 tabTop(topLeft.x + w * 0.35f, pos.y + padding);

        drawList->AddQuadFilled(tabStart, tabEnd, tabTop, tabStart, color);
        drawList->AddRectFilled(topLeft, bottomRight, color, 3.0f);
        drawList->AddRect(topLeft, bottomRight, outlineColor, 3.0f, 0, 2.0f);
    }
    else if (asset.extension == ".fbx" || asset.extension == ".obj")
    {
        float cubeSize = (size.x - padding * 2) * 0.6f;
        ImVec2 p1(center.x - cubeSize * 0.5f, center.y);
        ImVec2 p2(center.x + cubeSize * 0.5f, center.y);
        ImVec2 p3(center.x, center.y - cubeSize * 0.7f);
        ImVec2 p4(center.x, center.y + cubeSize * 0.7f);

        drawList->AddTriangleFilled(p1, p2, p3, color);
        drawList->AddTriangleFilled(p1, p2, p4, ImGui::ColorConvertFloat4ToU32(ImVec4(buttonColor.x * 0.7f, buttonColor.y * 0.7f, buttonColor.z * 0.7f, 1.0f)));
        drawList->AddTriangle(p1, p2, p3, outlineColor, 2.0f);
        drawList->AddTriangle(p1, p2, p4, outlineColor, 2.0f);
    }
    else if (asset.extension == ".png" || asset.extension == ".jpg" || asset.extension == ".jpeg" || asset.extension == ".dds")
    {
        float w = size.x - padding * 2;
        float h = size.y - padding * 2;
        ImVec2 topLeft(pos.x + padding, pos.y + padding);
        ImVec2 bottomRight(pos.x + size.x - padding, pos.y + size.y - padding);

        drawList->AddRectFilled(topLeft, bottomRight, color, 3.0f);
        drawList->AddRect(topLeft, bottomRight, outlineColor, 3.0f, 0, 2.0f);

        ImVec2 m1(topLeft.x + w * 0.2f, topLeft.y + h * 0.7f);
        ImVec2 m2(topLeft.x + w * 0.4f, topLeft.y + h * 0.4f);
        ImVec2 m3(topLeft.x + w * 0.6f, topLeft.y + h * 0.7f);
        drawList->AddTriangleFilled(m1, m2, m3, ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.3f, 0.8f)));

        drawList->AddCircleFilled(ImVec2(topLeft.x + w * 0.75f, topLeft.y + h * 0.25f), w * 0.1f, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 0.3f, 0.8f)));
    }
    else if (asset.extension == ".mesh")
    {
        // Icono de mesh individual (triángulo simple)
        float meshSize = (size.x - padding * 2) * 0.5f;
        ImVec2 p1(center.x, center.y - meshSize * 0.6f);
        ImVec2 p2(center.x - meshSize * 0.5f, center.y + meshSize * 0.4f);
        ImVec2 p3(center.x + meshSize * 0.5f, center.y + meshSize * 0.4f);

        drawList->AddTriangleFilled(p1, p2, p3, color);
        drawList->AddTriangle(p1, p2, p3, outlineColor, 2.0f);
    }
    else if (asset.extension == ".wav" || asset.extension == ".ogg" || asset.extension == ".mp3")
    {
        float w = size.x - padding * 2;
        float h = size.y - padding * 2;
        ImVec2 start(pos.x + padding, center.y);

        int bars = 5;
        float barWidth = w / (bars * 2);
        for (int i = 0; i < bars; i++)
        {
            float barHeight = h * 0.3f * (1.0f + sin(i * 0.8f) * 0.7f);
            ImVec2 p1(start.x + i * barWidth * 2, center.y - barHeight * 0.5f);
            ImVec2 p2(start.x + i * barWidth * 2 + barWidth, center.y + barHeight * 0.5f);
            drawList->AddRectFilled(p1, p2, color, 2.0f);
        }
    }
    else
    {
        float w = size.x - padding * 2;
        float h = size.y - padding * 2;
        ImVec2 topLeft(pos.x + padding, pos.y + padding);
        ImVec2 bottomRight(pos.x + size.x - padding, pos.y + size.y - padding);

        drawList->AddRectFilled(topLeft, bottomRight, color, 3.0f);
        drawList->AddRect(topLeft, bottomRight, outlineColor, 3.0f, 0, 2.0f);

        for (int i = 0; i < 3; i++)
        {
            float y = topLeft.y + h * 0.3f + i * h * 0.15f;
            drawList->AddLine(ImVec2(topLeft.x + w * 0.2f, y), ImVec2(bottomRight.x - w * 0.2f, y), outlineColor, 2.0f);
        }
    }
}

void AssetsWindow::Draw()
{
    if (!isOpen) return;

    static bool firstDraw = true;
    if (firstDraw) {
        RefreshAssets();
        firstDraw = false;
    }

    if (showDeleteConfirmation) {
        ImGui::OpenPopup("Delete Asset?");
        showDeleteConfirmation = false;
    }

    if (ImGui::BeginPopupModal("Delete Asset?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to delete:");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", assetToDelete.name.c_str());
        ImGui::Separator();

        if (assetToDelete.isDirectory) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "This will delete the entire folder and all its contents!");
        }
        else {
            ImGui::Text("This will also delete the corresponding Library file(s).");
        }

        ImGui::Separator();

        if (ImGui::Button("Delete", ImVec2(120, 0))) {
            if (DeleteAsset(assetToDelete)) {
                LOG_CONSOLE("Deleted: %s", assetToDelete.name.c_str());
                RefreshAssets();
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::Begin(name.c_str(), &isOpen))
    {
        isHovered = ImGui::IsWindowHovered();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

        if (ImGui::Button("Refresh"))
        {
            RefreshAssets();
        }

        ImGui::SameLine();
        ImGui::Checkbox("In Memory Only", &showInMemoryOnly);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::SliderFloat("Icon Size", &iconSize, 32.0f, 128.0f, "%.0f");

        ImGui::PopStyleVar();
        ImGui::Separator();

        ImGui::Text("Path: ");
        ImGui::SameLine();

        fs::path relativePath = fs::relative(currentPath, assetsRootPath);
        if (relativePath == ".")
        {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Assets");
        }
        else
        {
            if (ImGui::SmallButton("Assets##BreadcrumbRoot"))
            {
                currentPath = assetsRootPath;
                RefreshAssets();
            }

            std::string pathStr = relativePath.string();
            size_t pos = 0;
            std::string token;
            fs::path accumulatedPath = assetsRootPath;

            while ((pos = pathStr.find("\\")) != std::string::npos || (pos = pathStr.find("/")) != std::string::npos)
            {
                token = pathStr.substr(0, pos);
                ImGui::SameLine();
                ImGui::Text("/");
                ImGui::SameLine();

                accumulatedPath /= token;

                ImGui::PushID(accumulatedPath.string().c_str());
                if (ImGui::SmallButton(token.c_str()))
                {
                    currentPath = accumulatedPath.string();
                    RefreshAssets();
                }
                ImGui::PopID();

                pathStr.erase(0, pos + 1);
            }

            if (!pathStr.empty())
            {
                ImGui::SameLine();
                ImGui::Text("/");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), pathStr.c_str());
            }
        }

        ImGui::Separator();

        ImGui::BeginChild("FolderTree", ImVec2(200, 0), true);
        DrawFolderTree(assetsRootPath, "Assets");
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("AssetsList", ImVec2(0, 0), true);
        DrawAssetsList();
        ImGui::EndChild();
    }
    ImGui::End();
}

void AssetsWindow::DrawFolderTree(const fs::path& path, const std::string& label)
{
    if (!fs::exists(path) || !fs::is_directory(path))
        return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (path == currentPath)
        flags |= ImGuiTreeNodeFlags_Selected;

    bool hasSubfolders = false;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                hasSubfolders = true;
                break;
            }
        }
    }
    catch (...) {}

    if (!hasSubfolders)
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    ImGui::PushID(path.string().c_str());

    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        currentPath = path.string();
        RefreshAssets();
    }

    if (nodeOpen)
    {
        if (hasSubfolders)
        {
            try {
                for (const auto& entry : fs::directory_iterator(path))
                {
                    if (entry.is_directory())
                    {
                        std::string folderName = entry.path().filename().string();
                        DrawFolderTree(entry.path(), folderName);
                    }
                }
            }
            catch (...) {}

            ImGui::TreePop();
        }
    }

    ImGui::PopID();
}

void AssetsWindow::DrawAssetsList()
{
    if (currentPath != assetsRootPath)
    {
        if (ImGui::Button("<- Back"))
        {
            currentPath = fs::path(currentPath).parent_path().string();
            RefreshAssets();
        }
        ImGui::Separator();
    }

    float windowWidth = ImGui::GetContentRegionAvail().x;
    int columns = (int)(windowWidth / (iconSize + 10.0f));
    if (columns < 1) columns = 1;

    int currentColumn = 0;
    std::string pathPendingToLoad = "";

    for (auto& asset : currentAssets)
    {
        if (showInMemoryOnly && !asset.inMemory)
            continue;

        if (asset.isFBX) {
            DrawExpandableAssetItem(asset, pathPendingToLoad);
        }
        else {
            DrawAssetItem(asset, pathPendingToLoad);
        }

        currentColumn++;
        if (currentColumn < columns)
        {
            ImGui::SameLine();
        }
        else
        {
            currentColumn = 0;
        }
    }

    if (!pathPendingToLoad.empty())
    {
        currentPath = pathPendingToLoad;
        RefreshAssets();
    }
}

void AssetsWindow::DrawExpandableAssetItem(AssetEntry& asset, std::string& pathPendingToLoad)
{
    ImGui::PushID(asset.path.c_str());
    ImGui::BeginGroup();

    // Botón de flecha para expandir/colapsar
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    const char* arrowIcon = asset.isExpanded ? "v " : "> ";

    if (ImGui::SmallButton(arrowIcon))
    {
        asset.isExpanded = !asset.isExpanded;

        if (asset.isExpanded && asset.subMeshes.empty()) {
            LoadFBXSubMeshes(asset);
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Icono del FBX
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));

    bool clicked = ImGui::Button("##icon", ImVec2(iconSize, iconSize));

    ImGui::PopStyleColor(3);

    ImVec2 buttonPos = ImGui::GetItemRectMin();
    DrawIconShape(asset, buttonPos, ImVec2(iconSize, iconSize));

    bool isButtonHovered = ImGui::IsItemHovered();

    if (clicked)
    {
        selectedAsset = &asset;
    }

    std::string displayName = asset.name;
    if (!isButtonHovered)
    {
        displayName = TruncateFileName(asset.name, iconSize);
    }

    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize);
    ImGui::TextWrapped("%s", displayName.c_str());
    ImGui::PopTextWrapPos();

    if (asset.inMemory)
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded: %d", asset.references);
    }

    ImGui::EndGroup();

    // Context menu
    std::string popupID = "AssetContextMenu##" + asset.path;
    if (ImGui::BeginPopupContextItem(popupID.c_str()))
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", asset.name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            assetToDelete = asset;
            showDeleteConfirmation = true;
        }

        if (asset.uid != 0)
        {
            ImGui::Separator();
            ImGui::Text("UID: %llu", asset.uid);

            if (asset.inMemory)
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
            }
        }

        ImGui::EndPopup();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", asset.name.c_str());
        if (asset.uid != 0) {
            ImGui::Text("UID: %llu", asset.uid);
        }
        ImGui::Text("Contains %zu meshes", asset.subMeshes.size());
        ImGui::EndTooltip();
    }

    if (asset.isExpanded && !asset.subMeshes.empty())
    {
        // Continuar en la misma línea para expansión horizontal
        ImGui::SameLine();

        // Separador visual
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text(">");
        ImGui::PopStyleColor();

        // Dibujar cada submesh horizontalmente
        for (size_t i = 0; i < asset.subMeshes.size(); ++i)
        {
            ImGui::SameLine();

            auto& subMesh = asset.subMeshes[i];

            ImGui::PushID(subMesh.path.c_str());
            ImGui::BeginGroup();

            // Icono de la mesh (más pequeño)
            float smallIconSize = iconSize * 0.7f;

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));

            bool meshClicked = ImGui::Button("##meshicon", ImVec2(smallIconSize, smallIconSize));

            ImGui::PopStyleColor(3);

            ImVec2 meshButtonPos = ImGui::GetItemRectMin();
            DrawIconShape(subMesh, meshButtonPos, ImVec2(smallIconSize, smallIconSize));

            bool isMeshHovered = ImGui::IsItemHovered();

            if (meshClicked)
            {
                selectedAsset = &subMesh;
            }

            // Nombre de la mesh
            std::string meshDisplayName = subMesh.name;
            if (!isMeshHovered)
            {
                meshDisplayName = TruncateFileName(subMesh.name, smallIconSize);
            }

            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + smallIconSize);
            ImGui::TextWrapped("%s", meshDisplayName.c_str());
            ImGui::PopTextWrapPos();

            if (subMesh.inMemory)
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "R:%d", subMesh.references);
            }

            ImGui::EndGroup();

            // Context menu para submesh
            std::string meshPopupID = "MeshContextMenu##" + subMesh.path;
            if (ImGui::BeginPopupContextItem(meshPopupID.c_str()))
            {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", subMesh.name.c_str());
                ImGui::Separator();

                if (subMesh.uid != 0)
                {
                    ImGui::Text("UID: %llu", subMesh.uid);

                    if (subMesh.inMemory)
                    {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                    }
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Mesh: %s", subMesh.name.c_str());
                if (subMesh.uid != 0) {
                    ImGui::Text("UID: %llu", subMesh.uid);
                }
                if (subMesh.inMemory) {
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Refs: %d", subMesh.references);
                }
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }
    }

    ImGui::PopID();
}

void AssetsWindow::DrawAssetItem(const AssetEntry& asset, std::string& pathPendingToLoad)
{
    ImGui::PushID(asset.path.c_str());
    ImGui::BeginGroup();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));

    bool clicked = ImGui::Button("##icon", ImVec2(iconSize, iconSize));

    ImGui::PopStyleColor(3);

    ImVec2 buttonPos = ImGui::GetItemRectMin();
    DrawIconShape(asset, buttonPos, ImVec2(iconSize, iconSize));

    bool isButtonHovered = ImGui::IsItemHovered();

    if (clicked)
    {
        if (asset.isDirectory)
        {
            pathPendingToLoad = asset.path;
        }
        else
        {
            selectedAsset = const_cast<AssetEntry*>(&asset);
        }
    }

    if (isButtonHovered && ImGui::IsMouseDoubleClicked(0))
    {
        if (asset.isDirectory)
        {
            pathPendingToLoad = asset.path;
        }
    }

    std::string displayName = asset.name;
    if (!isButtonHovered)
    {
        displayName = TruncateFileName(asset.name, iconSize);
    }

    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize);
    ImGui::TextWrapped("%s", displayName.c_str());
    ImGui::PopTextWrapPos();

    if (asset.inMemory)
    {
        if (asset.isDirectory) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded: %d", asset.references);
        }
        else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Refs: %d", asset.references);
        }
    }

    ImGui::EndGroup();

    std::string popupID = "AssetContextMenu##" + asset.path;
    if (ImGui::BeginPopupContextItem(popupID.c_str()))
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", asset.name.c_str());
        ImGui::Separator();

        if (ImGui::MenuItem("Delete"))
        {
            assetToDelete = asset;
            showDeleteConfirmation = true;
        }

        if (!asset.isDirectory && asset.uid != 0)
        {
            ImGui::Separator();
            ImGui::Text("UID: %llu", asset.uid);

            if (asset.inMemory)
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
            }
        }

        if (asset.isDirectory && asset.inMemory)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Contains loaded assets");
            ImGui::Text("Total refs: %d", asset.references);
        }

        ImGui::EndPopup();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", asset.name.c_str());
        if (!asset.isDirectory && asset.uid != 0) {
            ImGui::Text("UID: %llu", asset.uid);
        }
        if (asset.isDirectory && asset.inMemory) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Contains %d loaded refs", asset.references);
        }
        ImGui::EndTooltip();
    }

    ImGui::PopID();
}

void AssetsWindow::LoadFBXSubMeshes(AssetEntry& fbxAsset)
{
    fbxAsset.subMeshes.clear();

    // Cargar el .meta del FBX
    std::string metaPath = fbxAsset.path + ".meta";
    if (!fs::exists(metaPath)) {
        LOG_DEBUG("[AssetsWindow] No .meta file found for FBX: %s", fbxAsset.path.c_str());
        return;
    }

    MetaFile meta = MetaFile::Load(metaPath);

    if (meta.libraryPaths.empty()) {
        LOG_DEBUG("[AssetsWindow] FBX has no library paths in .meta");
        return;
    }

    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        LOG_DEBUG("[AssetsWindow] ModuleResources not available");
        return;
    }

    // Crear una entrada AssetEntry por cada mesh en libraryPaths
    int meshIndex = 0;
    for (const std::string& libPath : meta.libraryPaths)
    {
        if (libPath.empty()) continue;

        AssetEntry meshEntry;

        // Extraer nombre del archivo mesh
        fs::path meshPath(libPath);
        meshEntry.name = meshPath.stem().string(); // Sin extensión

        // Si el nombre está vacío o es genérico, usar índice
        if (meshEntry.name.empty() || meshEntry.name == "unnamed_mesh") {
            meshEntry.name = "Mesh_" + std::to_string(meshIndex);
        }

        meshEntry.path = libPath;
        meshEntry.extension = ".mesh";
        meshEntry.isDirectory = false;
        meshEntry.isFBX = false;
        meshEntry.isExpanded = false;
        meshEntry.inMemory = false;
        meshEntry.references = 0;
        meshEntry.uid = 0;

        // Buscar el UID correspondiente en resources
        const auto& allResources = resources->GetAllResources();
        for (const auto& pair : allResources)
        {
            if (pair.second->GetLibraryFile() == libPath)
            {
                meshEntry.uid = pair.first;

                if (pair.second->IsLoadedToMemory())
                {
                    meshEntry.inMemory = true;
                    meshEntry.references = pair.second->GetReferenceCount();
                }

                break;
            }
        }

        fbxAsset.subMeshes.push_back(meshEntry);
        meshIndex++;
    }

    LOG_DEBUG("[AssetsWindow] Loaded %zu submeshes for FBX: %s",
        fbxAsset.subMeshes.size(), fbxAsset.name.c_str());
}

bool AssetsWindow::DeleteAsset(const AssetEntry& asset)
{
    try {
        if (asset.isDirectory)
        {
            return DeleteDirectory(fs::path(asset.path));
        }
        else
        {
            std::string metaPath = asset.path + ".meta";
            unsigned long long uid = 0;
            std::vector<std::string> libraryPaths;

            if (fs::exists(metaPath)) {
                MetaFile meta = MetaFile::Load(metaPath);
                uid = meta.uid;
                libraryPaths = meta.GetAllLibraryPaths();
            }

            for (const auto& libPath : libraryPaths) {
                if (!libPath.empty() && fs::exists(libPath)) {
                    fs::remove(libPath);
                }
            }

            if (fs::exists(metaPath)) {
                fs::remove(metaPath);
            }

            if (fs::exists(asset.path)) {
                fs::remove(asset.path);
            }

            if (uid != 0 && Application::GetInstance().resources) {
                Application::GetInstance().resources->RemoveResource(uid);
            }

            return true;
        }
    }
    catch (const fs::filesystem_error& e) {
        return false;
    }
}

bool AssetsWindow::DeleteDirectory(const fs::path& dirPath)
{
    try {
        std::vector<std::pair<unsigned long long, std::vector<std::string>>> filesToDelete;

        for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".meta") {
                MetaFile meta = MetaFile::Load(entry.path().string());
                if (meta.uid != 0) {
                    filesToDelete.push_back({ meta.uid, meta.GetAllLibraryPaths() });
                }
            }
        }

        for (const auto& [uid, libraryPaths] : filesToDelete) {
            for (const auto& libPath : libraryPaths) {
                if (!libPath.empty() && fs::exists(libPath)) {
                    fs::remove(libPath);
                }
            }

            if (Application::GetInstance().resources) {
                Application::GetInstance().resources->RemoveResource(uid);
            }
        }

        fs::remove_all(dirPath);

        return true;
    }
    catch (const fs::filesystem_error& e) {
        return false;
    }
}

void AssetsWindow::RefreshAssets()
{
    currentAssets.clear();

    if (!fs::exists(currentPath))
    {
        return;
    }

    ScanDirectory(currentPath, currentAssets);

}

void AssetsWindow::ScanDirectory(const fs::path& directory, std::vector<AssetEntry>& outAssets)
{
    if (!fs::exists(directory))
        return;

    for (const auto& entry : fs::directory_iterator(directory))
    {
        const auto& path = entry.path();
        std::string filename = path.filename().string();
        std::string extension = path.extension().string();

        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (extension == ".meta")
        {
            continue;
        }

        bool isDirectory = entry.is_directory();

        if (!isDirectory && !IsAssetFile(extension))
        {
            continue;
        }

        AssetEntry asset;
        asset.name = filename;
        asset.path = path.string();
        asset.isDirectory = isDirectory;
        asset.extension = extension;
        asset.inMemory = false;
        asset.references = 0;
        asset.uid = 0;

        asset.isFBX = (extension == ".fbx");
        asset.isExpanded = false;
        asset.subMeshes.clear();

        if (isDirectory)
        {
            ModuleResources* resources = Application::GetInstance().resources.get();
            if (resources)
            {
                int totalRefs = 0;
                bool anyLoaded = false;

                try {
                    for (const auto& subEntry : fs::recursive_directory_iterator(path))
                    {
                        if (subEntry.is_regular_file())
                        {
                            std::string subExt = subEntry.path().extension().string();
                            std::transform(subExt.begin(), subExt.end(), subExt.begin(),
                                [](unsigned char c) { return std::tolower(c); });

                            if (subExt == ".meta")
                                continue;

                            if (!IsAssetFile(subExt))
                                continue;

                            std::string subMetaPath = subEntry.path().string() + ".meta";
                            if (fs::exists(subMetaPath))
                            {
                                MetaFile subMeta = MetaFile::Load(subMetaPath);
                                if (subMeta.uid != 0)
                                {
                                    if (subExt == ".fbx")
                                    {
                                        const auto& allResources = resources->GetAllResources();
                                        for (const std::string& libPath : subMeta.libraryPaths)
                                        {
                                            if (libPath.empty()) continue;

                                            for (const auto& pair : allResources)
                                            {
                                                if (pair.second->GetLibraryFile() == libPath)
                                                {
                                                    if (pair.second->IsLoadedToMemory())
                                                    {
                                                        anyLoaded = true;
                                                        totalRefs += pair.second->GetReferenceCount();
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if (resources->IsResourceLoaded(subMeta.uid))
                                        {
                                            anyLoaded = true;
                                            totalRefs += resources->GetResourceReferenceCount(subMeta.uid);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                catch (...) {}

                asset.inMemory = anyLoaded;
                asset.references = totalRefs;
            }
        }
        else
        {
            std::string metaPath = asset.path + ".meta";
            if (fs::exists(metaPath))
            {
                MetaFile meta = MetaFile::Load(metaPath);
                asset.uid = meta.uid;

                if (asset.uid != 0 && Application::GetInstance().resources)
                {
                    ModuleResources* resources = Application::GetInstance().resources.get();

                    if (extension == ".fbx")
                    {
                        int totalRefs = 0;
                        bool anyLoaded = false;

                        const auto& allResources = resources->GetAllResources();

                        for (const std::string& libPath : meta.libraryPaths)
                        {
                            if (libPath.empty()) continue;

                            for (const auto& pair : allResources)
                            {
                                if (pair.second->GetLibraryFile() == libPath)
                                {
                                    if (pair.second->IsLoadedToMemory())
                                    {
                                        anyLoaded = true;
                                        int refs = pair.second->GetReferenceCount();
                                        totalRefs += refs;
                                    }
                                    break;
                                }
                            }
                        }

                        asset.inMemory = anyLoaded;
                        asset.references = totalRefs;
                    }
                    else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                        extension == ".dds" || extension == ".tga")
                    {
                        asset.inMemory = resources->IsResourceLoaded(asset.uid);
                        asset.references = resources->GetResourceReferenceCount(asset.uid);
                    }
                    else
                    {
                        asset.inMemory = resources->IsResourceLoaded(asset.uid);
                        asset.references = resources->GetResourceReferenceCount(asset.uid);
                    }
                }
                else
                {
                    if (asset.uid == 0) {
                        LOG_CONSOLE("  ERROR: UID is 0!");
                    }
                    if (!Application::GetInstance().resources) {
                        LOG_CONSOLE("  ERROR: ModuleResources is NULL!");
                    }
                }
            }
            else
            {
                LOG_CONSOLE("  WARNING: No meta file found!");
            }
        }

        outAssets.push_back(asset);
    }

    std::sort(outAssets.begin(), outAssets.end(), [](const AssetEntry& a, const AssetEntry& b)
        {
            if (a.isDirectory != b.isDirectory)
                return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
}

const char* AssetsWindow::GetAssetIcon(const std::string& extension) const
{
    if (extension.empty()) return "[DIR]";
    if (extension == ".fbx" || extension == ".obj") return "[3D]";
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".dds") return "[IMG]";
    if (extension == ".mesh") return "[MSH]";
    if (extension == ".texture") return "[TEX]";
    if (extension == ".wav" || extension == ".ogg" || extension == ".mp3") return "[SND]";
    return "[FILE]";
}

bool AssetsWindow::IsAssetFile(const std::string& extension) const
{
    return extension == ".fbx" ||
        extension == ".obj" ||
        extension == ".png" ||
        extension == ".jpg" ||
        extension == ".jpeg" ||
        extension == ".dds" ||
        extension == ".tga" ||
        extension == ".mesh" ||
        extension == ".texture" ||
        extension == ".wav" ||
        extension == ".ogg";
}