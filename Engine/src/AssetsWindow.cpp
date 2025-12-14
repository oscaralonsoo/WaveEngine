#include "AssetsWindow.h"
#include <imgui.h>
#include "Application.h"
#include "LibraryManager.h"
#include "MetaFile.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "ResourceMesh.h"
#include "Log.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ImportSettingsWindow.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

AssetsWindow::AssetsWindow()
    : EditorWindow("Assets"), selectedAsset(nullptr), iconSize(64.0f),
    showInMemoryOnly(false), showDeleteConfirmation(false)
{
    if (!LibraryManager::IsInitialized()) {
        LibraryManager::Initialize();
    }

    assetsRootPath = LibraryManager::GetAssetsRoot();
    currentPath = assetsRootPath;

    sceneRootPath = fs::path(assetsRootPath).parent_path().string() + "/Scene";
    importSettingsWindow = new ImportSettingsWindow();
}

AssetsWindow::~AssetsWindow()
{
    // Limpiar todos los previews
    for (auto& asset : currentAssets)
    {
        UnloadPreviewForAsset(asset);
    }
    delete importSettingsWindow;
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

    // Si hay preview texture, mostrarla
    if (asset.previewTextureID != 0)
    {
        ImVec2 topLeft = pos;
        ImVec2 bottomRight = ImVec2(pos.x + size.x, pos.y + size.y);

        ImTextureID texID = (ImTextureID)(uintptr_t)asset.previewTextureID;
        drawList->AddImage(texID, topLeft, bottomRight, ImVec2(0, 1), ImVec2(1, 0));

        // Borde alrededor del preview
        ImU32 borderColor = asset.inMemory ?
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.8f, 0.3f, 1.0f)) :
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        drawList->AddRect(topLeft, bottomRight, borderColor, 3.0f, 0, 2.0f);
        return;
    }

    // Si no hay preview, usar los iconos dibujados originales
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

        fs::path activeRoot;
        std::string rootName;

        // Detectar en qué rama estamos
        bool isInScene = (currentPath.find(sceneRootPath) != std::string::npos);

        if (isInScene) {
            activeRoot = sceneRootPath;
            rootName = "Scene";
        }
        else {
            activeRoot = assetsRootPath;
            rootName = "Assets";
        }

        fs::path relativePath = fs::relative(currentPath, activeRoot);

        if (relativePath == ".")
        {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), rootName.c_str());
        }
        else
        {
            // Botón para volver a la raíz activa (Assets o Scene)
            if (ImGui::SmallButton((rootName + "##BreadcrumbRoot").c_str()))
            {
                currentPath = activeRoot.string();
                RefreshAssets();
            }

            std::string pathStr = relativePath.string();
            size_t pos = 0;
            std::string token;
            fs::path accumulatedPath = activeRoot; // Usar activeRoot para la acumulación

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
        DrawFolderTree(sceneRootPath, "Scene");
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("AssetsList", ImVec2(0, 0), true);
        DrawAssetsList();
        ImGui::EndChild();
    }

    if (importSettingsWindow)
    {
        importSettingsWindow->Draw();
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
    if (currentPath != assetsRootPath && currentPath != sceneRootPath)
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

        // Cargar preview si aún no se ha cargado
        if (!asset.isDirectory && !asset.previewLoaded)
        {
            LoadPreviewForAsset(asset);
        }

        // Solo FBX se expanden
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

    // Drag & Drop source for FBX
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        static DragDropPayload payload; // Hacer estático para que persista
        payload.assetPath = asset.path;
        payload.assetUID = asset.uid;
        payload.assetType = DragDropAssetType::FBX_MODEL;

        ImGui::SetDragDropPayload("ASSET_ITEM", &payload, sizeof(DragDropPayload));
        ImGui::Text("FBX: %s", asset.name.c_str());
        ImGui::EndDragDropSource();
    }

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

        // Import Settings
        if (ImGui::MenuItem("Import Settings..."))
        {
            if (importSettingsWindow)
            {
                importSettingsWindow->OpenForAsset(asset.path);
            }
        }
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

    // Dibujar submeshes horizontalmente con wrapping
    if (asset.isExpanded && !asset.subMeshes.empty())
    {
        float smallIconSize = iconSize * 0.7f;
        float itemWidth = smallIconSize + 15.0f;

        LOG_DEBUG("[DrawExpandableAsset] FBX: %s has %zu submeshes", asset.name.c_str(), asset.subMeshes.size());

        ImGui::Dummy(ImVec2(0, 0));

        // Calcular indentación para las submeshes
        float indentSize = 30.0f;
        float startX = ImGui::GetCursorPosX() + indentSize;

        // Separador visual con indentación
        ImGui::SetCursorPosX(startX);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text(">");
        ImGui::PopStyleColor();

        // Obtener el ancho disponible considerando la indentación
        float windowContentWidth = ImGui::GetContentRegionAvail().x;
        float currentX = startX;

        LOG_DEBUG("[DrawExpandableAsset] startX: %.2f, windowContentWidth: %.2f, itemWidth: %.2f",
            startX, windowContentWidth, itemWidth);

        // Dibujar cada submesh
        for (size_t i = 0; i < asset.subMeshes.size(); ++i)
        {
            auto& subMesh = asset.subMeshes[i];

            // Cargar preview si aún no se ha cargado
            if (!subMesh.previewLoaded)
            {
                LoadPreviewForAsset(subMesh);
            }

            // Verificar si necesitamos nueva línea
            if (i > 0)
            {
                float remainingWidth = startX + windowContentWidth - currentX;

                LOG_DEBUG("[DrawExpandableAsset] Mesh %zu: currentX: %.2f, remaining: %.2f",
                    i, currentX, remainingWidth);

                if (remainingWidth < itemWidth)
                {
                    // Nueva línea con indentación
                    LOG_DEBUG("[DrawExpandableAsset] Mesh %zu: NUEVA LINEA", i);
                    ImGui::NewLine();
                    ImGui::SetCursorPosX(startX);
                    currentX = startX;
                }
                else
                {
                    // Continuar en la misma línea
                    ImGui::SameLine();
                }
            }
            else
            {
                // Primera submesh - continuar en la misma línea del separador
                ImGui::SameLine();
            }

            // Actualizar posición actual
            currentX = ImGui::GetCursorPosX();

            ImGui::PushID(subMesh.path.c_str());
            ImGui::BeginGroup();

            // Icono de la mesh (más pequeño)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.4f));

            bool meshClicked = ImGui::Button("##meshicon", ImVec2(smallIconSize, smallIconSize));

            ImGui::PopStyleColor(3);

            ImVec2 meshButtonPos = ImGui::GetItemRectMin();
            DrawIconShape(subMesh, meshButtonPos, ImVec2(smallIconSize, smallIconSize));

            bool isMeshHovered = ImGui::IsItemHovered();

            // Drag & Drop source for individual mesh
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                static DragDropPayload payload; // Hacer estático para que persista
                payload.assetPath = subMesh.path;
                payload.assetUID = subMesh.uid;
                payload.assetType = DragDropAssetType::MESH;

                ImGui::SetDragDropPayload("ASSET_ITEM", &payload, sizeof(DragDropPayload));
                ImGui::Text("Mesh: %s", subMesh.name.c_str());
                ImGui::EndDragDropSource();
            }

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

            // Actualizar currentX después del grupo (incluye el ancho del elemento)
            currentX += itemWidth;

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

    // Drag & Drop source (solo para archivos, no directorios)
    if (!asset.isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        static DragDropPayload payload; // Hacer estático para que persista
        payload.assetPath = asset.path;
        payload.assetUID = asset.uid;

        // Determinar el tipo de asset
        if (asset.extension == ".png" || asset.extension == ".jpg" ||
            asset.extension == ".jpeg" || asset.extension == ".dds" || asset.extension == ".tga")
        {
            payload.assetType = DragDropAssetType::TEXTURE;
            ImGui::Text("Texture: %s", asset.name.c_str());
        }
        else if (asset.extension == ".mesh")
        {
            payload.assetType = DragDropAssetType::MESH;
            ImGui::Text("Mesh: %s", asset.name.c_str());
        }
        else
        {
            payload.assetType = DragDropAssetType::UNKNOWN;
            ImGui::Text("Drag: %s", asset.name.c_str());
        }

        ImGui::SetDragDropPayload("ASSET_ITEM", &payload, sizeof(DragDropPayload));
        ImGui::EndDragDropSource();
    }

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

        if (!asset.isDirectory &&
            (asset.extension == ".fbx" ||
                asset.extension == ".png" || asset.extension == ".jpg" ||
                asset.extension == ".jpeg" || asset.extension == ".dds" ||
                asset.extension == ".tga"))
        {
            if (ImGui::MenuItem("Import Settings..."))
            {
                if (importSettingsWindow)
                {
                    importSettingsWindow->OpenForAsset(asset.path);
                }
            }
            ImGui::Separator();
        }

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

    std::string metaPath = fbxAsset.path + ".meta";
    if (!fs::exists(metaPath)) {
        LOG_CONSOLE("[AssetsWindow] No .meta file found for FBX: %s", fbxAsset.path.c_str());
        return;
    }

    MetaFile meta = MetaFile::Load(metaPath);

    if (meta.uid == 0) {
        LOG_CONSOLE("[AssetsWindow] FBX has no UID in .meta");
        return;
    }

    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) {
        LOG_CONSOLE("[AssetsWindow] ModuleResources not available");
        return;
    }

    const auto& allResources = resources->GetAllResources();

    int meshIndex = 0;
    const int maxMeshes = 100; // Safety limit

    for (int i = 0; i < maxMeshes; i++)
    {
        unsigned long long meshUID = meta.uid + i;
        std::string libPath = LibraryManager::GetMeshPathFromUID(meshUID);

        // Check if file exists
        if (!LibraryManager::FileExists(libPath)) {
            break; // No more meshes
        }

        AssetEntry meshEntry;

        fs::path meshPath(libPath);
        meshEntry.name = "Mesh_" + std::to_string(meshIndex);

        meshEntry.path = libPath;
        meshEntry.extension = ".mesh";
        meshEntry.isDirectory = false;
        meshEntry.isFBX = false;
        meshEntry.isExpanded = false;
        meshEntry.uid = meshUID;
        meshEntry.inMemory = false;
        meshEntry.references = 0;
        meshEntry.previewTextureID = 0;
        meshEntry.previewLoaded = false;

        // Check if loaded in memory
        auto it = allResources.find(meshUID);
        if (it != allResources.end())
        {
            meshEntry.inMemory = it->second->IsLoadedToMemory();
            meshEntry.references = it->second->GetReferenceCount();
        }

        fbxAsset.subMeshes.push_back(meshEntry);
        meshIndex++;
    }

    LOG_DEBUG("[AssetsWindow] Loaded %d submeshes for FBX: %s", meshIndex, fbxAsset.name.c_str());
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

            if (fs::exists(metaPath)) {
                MetaFile meta = MetaFile::Load(metaPath);
                uid = meta.uid;

                // For FBX files, delete all sequential mesh files
                if (meta.type == AssetType::MODEL_FBX) {
                    // Delete all mesh files with UIDs: base_uid, base_uid+1, base_uid+2, etc.
                    for (int i = 0; i < 100; i++) {
                        unsigned long long meshUID = uid + i;
                        std::string libPath = LibraryManager::GetMeshPathFromUID(meshUID);

                        if (fs::exists(libPath)) {
                            fs::remove(libPath);
                        }
                        else {
                            break; // No more meshes
                        }
                    }
                }
                else {
                    // For textures and other single-file assets
                    std::string libPath;

                    if (meta.type == AssetType::TEXTURE_PNG ||
                        meta.type == AssetType::TEXTURE_JPG ||
                        meta.type == AssetType::TEXTURE_DDS ||
                        meta.type == AssetType::TEXTURE_TGA) {
                        libPath = LibraryManager::GetTexturePathFromUID(uid);
                    }

                    if (!libPath.empty() && fs::exists(libPath)) {
                        fs::remove(libPath);
                    }
                }
            }

            // Delete .meta file
            if (fs::exists(metaPath)) {
                fs::remove(metaPath);
            }

            // Delete asset file
            if (fs::exists(asset.path)) {
                fs::remove(asset.path);
            }

            // Remove from ModuleResources
            if (uid != 0 && Application::GetInstance().resources) {
                Application::GetInstance().resources->RemoveResource(uid);
            }

            return true;
        }
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[AssetsWindow] ERROR deleting asset: %s", e.what());
        return false;
    }
}

bool AssetsWindow::DeleteDirectory(const fs::path& dirPath)
{
    try {
        std::vector<unsigned long long> uidsToDelete;

        // Collect all UIDs from .meta files in directory
        for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".meta") {
                MetaFile meta = MetaFile::Load(entry.path().string());
                if (meta.uid != 0) {
                    uidsToDelete.push_back(meta.uid);

                    // For FBX files, also delete all mesh UIDs
                    if (meta.type == AssetType::MODEL_FBX) {
                        for (int i = 0; i < 100; i++) {
                            unsigned long long meshUID = meta.uid + i;
                            std::string libPath = LibraryManager::GetMeshPathFromUID(meshUID);

                            if (fs::exists(libPath)) {
                                fs::remove(libPath);
                            }
                            else {
                                break;
                            }
                        }
                    }
                    else {
                        // Delete single library file
                        std::string libPath;

                        if (meta.type == AssetType::TEXTURE_PNG ||
                            meta.type == AssetType::TEXTURE_JPG ||
                            meta.type == AssetType::TEXTURE_DDS ||
                            meta.type == AssetType::TEXTURE_TGA) {
                            libPath = LibraryManager::GetTexturePathFromUID(meta.uid);
                        }

                        if (!libPath.empty() && fs::exists(libPath)) {
                            fs::remove(libPath);
                        }
                    }
                }
            }
        }

        // Remove from ModuleResources
        if (Application::GetInstance().resources) {
            for (unsigned long long uid : uidsToDelete) {
                Application::GetInstance().resources->RemoveResource(uid);
            }
        }

        // Delete entire directory
        fs::remove_all(dirPath);

        return true;
    }
    catch (const fs::filesystem_error& e) {
        LOG_CONSOLE("[AssetsWindow] ERROR deleting directory: %s", e.what());
        return false;
    }
}

void AssetsWindow::RefreshAssets()
{
    // Limpiar previews antiguos
    for (auto& asset : currentAssets)
    {
        UnloadPreviewForAsset(asset);
    }

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
                                        // Para FBX, verificar todas las meshes con UIDs secuenciales
                                        const auto& allResources = resources->GetAllResources();

                                        for (int i = 0; i < 100; i++) {
                                            unsigned long long meshUID = subMeta.uid + i;
                                            std::string meshLibPath = LibraryManager::GetMeshPathFromUID(meshUID);

                                            if (!fs::exists(meshLibPath)) {
                                                break; // No más meshes
                                            }

                                            // Verificar si está cargada en memoria
                                            for (const auto& pair : allResources)
                                            {
                                                if (pair.second->GetLibraryFile() == meshLibPath)
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
                                    else if (subExt == ".png" || subExt == ".jpg" || subExt == ".jpeg" ||
                                        subExt == ".dds" || subExt == ".tga")
                                    {
                                        if (resources->IsResourceLoaded(subMeta.uid))
                                        {
                                            anyLoaded = true;
                                            totalRefs += resources->GetResourceReferenceCount(subMeta.uid);
                                        }
                                    }
                                    else
                                    {
                                        // Para otros tipos de assets
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

                        // Verificar todas las meshes del FBX (UIDs secuenciales)
                        for (int i = 0; i < 100; i++) {
                            unsigned long long meshUID = meta.uid + i;
                            std::string meshLibPath = LibraryManager::GetMeshPathFromUID(meshUID);

                            if (!fs::exists(meshLibPath)) {
                                break; // No más meshes
                            }

                            // Verificar si está cargada en memoria
                            for (const auto& pair : allResources)
                            {
                                if (pair.second->GetLibraryFile() == meshLibPath)
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
        extension == ".ogg" ||
        extension == ".json";
}

void AssetsWindow::LoadPreviewForAsset(AssetEntry& asset)
{
    // Si ya se intentó cargar, no volver a intentar
    if (asset.previewLoaded)
        return;

    asset.previewLoaded = true;

    // Texturas (PNG, JPG, JPEG, TGA)
    if (asset.extension == ".png" || asset.extension == ".jpg" ||
        asset.extension == ".jpeg" || asset.extension == ".tga")
    {
        // Cargar imagen directamente con stb_image
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);

        unsigned char* data = stbi_load(asset.path.c_str(), &width, &height, &channels, 0);

        if (data)
        {
            // Crear textura OpenGL
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            // Configurar parámetros
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Determinar formato
            GLenum format = GL_RGB;
            if (channels == 1)
                format = GL_RED;
            else if (channels == 3)
                format = GL_RGB;
            else if (channels == 4)
                format = GL_RGBA;

            // Subir datos
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            glBindTexture(GL_TEXTURE_2D, 0);

            // Liberar datos
            stbi_image_free(data);

            // Guardar ID
            asset.previewTextureID = textureID;

            LOG_DEBUG("[AssetsWindow] Loaded preview for texture: %s", asset.name.c_str());
        }
        else
        {
            LOG_DEBUG("[AssetsWindow] Failed to load preview for: %s", asset.name.c_str());
        }
    }
    // Texturas DDS
    else if (asset.extension == ".dds")
    {
        // Para DDS, usar el sistema de recursos existente
        if (asset.uid != 0)
        {
            ModuleResources* resources = Application::GetInstance().resources.get();
            if (resources)
            {
                const Resource* resource = resources->GetResource(asset.uid);
                if (resource && resource->IsLoadedToMemory())
                {
                    const ResourceTexture* texResource = dynamic_cast<const ResourceTexture*>(resource);
                    if (texResource)
                    {
                        asset.previewTextureID = texResource->GetGPU_ID();
                        LOG_DEBUG("[AssetsWindow] Using existing texture for preview: %s", asset.name.c_str());
                    }
                }
            }
        }
    }
    // Meshes individuales
    else if (asset.extension == ".mesh")
    {
        if (asset.uid != 0)
        {
            ModuleResources* resources = Application::GetInstance().resources.get();
            if (resources)
            {
                // Cargar la mesh temporalmente
                ResourceMesh* meshResource = dynamic_cast<ResourceMesh*>(
                    resources->RequestResource(asset.uid)
                    );

                if (meshResource && meshResource->IsLoadedToMemory())
                {
                    const Mesh& mesh = meshResource->GetMesh();

                    // Renderizar a textura
                    int previewSize = static_cast<int>(iconSize * 2);
                    asset.previewTextureID = RenderMeshToTexture(mesh, previewSize, previewSize);

                    LOG_DEBUG("[AssetsWindow] Rendered preview for mesh: %s", asset.name.c_str());

                    // Liberar recurso
                    resources->ReleaseResource(asset.uid);
                }
            }
        }
    }
    // FBX (renderizar TODAS las meshes juntas)
    else if (asset.extension == ".fbx")
    {
        if (asset.uid != 0)
        {
            ModuleResources* resources = Application::GetInstance().resources.get();
            if (resources)
            {
                // Cargar todas las meshes del FBX
                std::vector<const Mesh*> meshes;

                // Intentar cargar hasta 100 meshes (UID secuenciales)
                for (int i = 0; i < 100; i++)
                {
                    unsigned long long meshUID = asset.uid + i;
                    std::string meshLibPath = LibraryManager::GetMeshPathFromUID(meshUID);

                    if (!LibraryManager::FileExists(meshLibPath))
                    {
                        break; // No más meshes
                    }

                    ResourceMesh* meshResource = dynamic_cast<ResourceMesh*>(
                        resources->RequestResource(meshUID)
                        );

                    if (meshResource && meshResource->IsLoadedToMemory())
                    {
                        meshes.push_back(&meshResource->GetMesh());
                    }
                }

                if (!meshes.empty())
                {
                    // Renderizar todas las meshes juntas
                    int previewSize = static_cast<int>(iconSize * 2);
                    asset.previewTextureID = RenderMultipleMeshesToTexture(meshes, previewSize, previewSize);

                    LOG_DEBUG("[AssetsWindow] Rendered preview for FBX: %s (%zu meshes)",
                        asset.name.c_str(), meshes.size());

                    // Liberar recursos
                    for (int i = 0; i < static_cast<int>(meshes.size()); i++)
                    {
                        resources->ReleaseResource(asset.uid + i);
                    }
                }
            }
        }
    }
}

void AssetsWindow::UnloadPreviewForAsset(AssetEntry& asset)
{
    // Liberar texturas que cargamos nosotros (no las del resource system)
    if (asset.previewTextureID != 0)
    {
        // Solo liberamos si no es una textura DDS del resource system
        if (asset.extension != ".dds")
        {
            glDeleteTextures(1, &asset.previewTextureID);
        }
        asset.previewTextureID = 0;
    }

    asset.previewLoaded = false;
}

unsigned int AssetsWindow::RenderMeshToTexture(const Mesh& mesh, int width, int height)
{
    // Guardar estado actual de OpenGL
    GLint oldFBO, oldViewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    // Crear framebuffer para renderizar off-screen
    GLuint fbo, colorTexture, depthRBO;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Crear textura para el color
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Crear renderbuffer para depth
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    // Verificar que el framebuffer está completo
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_DEBUG("[RenderMeshToTexture] ERROR: Framebuffer incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &depthRBO);
        glDeleteTextures(1, &colorTexture);
        return 0;
    }

    // Configurar viewport
    glViewport(0, 0, width, height);

    // Limpiar con fondo gris oscuro
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Habilitar depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Calcular AABB de la mesh para posicionar la cámara
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);

    for (const auto& vertex : mesh.vertices)
    {
        minBounds = glm::min(minBounds, vertex.position);
        maxBounds = glm::max(maxBounds, vertex.position);
    }

    glm::vec3 center = (minBounds + maxBounds) * 0.5f;
    glm::vec3 size = maxBounds - minBounds;
    float maxDim = glm::max(size.x, glm::max(size.y, size.z));

    // Configurar matrices de cámara
    float distance = maxDim * 2.2f;
    glm::vec3 cameraPos = center + glm::vec3(distance * 0.6f, distance * 0.4f, distance * 0.6f);

    glm::mat4 view = glm::lookAt(cameraPos, center, glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, distance * 10.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    // Shader simple inline
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 mvp;
        out vec3 Normal;
        
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
            Normal = aNormal;
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 Normal;
        out vec4 FragColor;
        
        void main() {
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            vec3 norm = normalize(Normal);
            float diff = max(dot(norm, lightDir), 0.0);
            
            vec3 baseColor = vec3(0.7, 0.7, 0.75);
            vec3 ambient = 0.3 * baseColor;
            vec3 diffuse = diff * baseColor;
            
            FragColor = vec4(ambient + diffuse, 1.0);
        }
    )";

    // Compilar shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Usar shader
    glUseProgram(shaderProgram);

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    if (mvpLoc != -1)
    {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    }

    // Renderizar la mesh
    if (mesh.VAO != 0 && !mesh.indices.empty())
    {
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Limpiar shader
    glDeleteProgram(shaderProgram);

    // Restaurar estado
    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    // Limpiar framebuffer y depth buffer (pero mantener la textura)
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depthRBO);

    return colorTexture;
}

unsigned int AssetsWindow::RenderMultipleMeshesToTexture(const std::vector<const Mesh*>& meshes, int width, int height)
{
    if (meshes.empty())
        return 0;

    // Guardar estado actual de OpenGL
    GLint oldFBO, oldViewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
    glGetIntegerv(GL_VIEWPORT, oldViewport);

    // Crear framebuffer para renderizar off-screen
    GLuint fbo, colorTexture, depthRBO;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Crear textura para el color
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    // Crear renderbuffer para depth
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

    // Verificar que el framebuffer está completo
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_DEBUG("[RenderMultipleMeshesToTexture] ERROR: Framebuffer incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &depthRBO);
        glDeleteTextures(1, &colorTexture);
        return 0;
    }

    // Configurar viewport
    glViewport(0, 0, width, height);

    // Limpiar con fondo gris oscuro
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Habilitar depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Calcular AABB global de todas las meshes combinadas
    glm::vec3 globalMinBounds(FLT_MAX);
    glm::vec3 globalMaxBounds(-FLT_MAX);

    for (const Mesh* mesh : meshes)
    {
        for (const auto& vertex : mesh->vertices)
        {
            globalMinBounds = glm::min(globalMinBounds, vertex.position);
            globalMaxBounds = glm::max(globalMaxBounds, vertex.position);
        }
    }

    glm::vec3 center = (globalMinBounds + globalMaxBounds) * 0.5f;
    glm::vec3 size = globalMaxBounds - globalMinBounds;
    float maxDim = glm::max(size.x, glm::max(size.y, size.z));

    // Configurar matrices de cámara
    float distance = maxDim * 2.2f;
    glm::vec3 cameraPos = center + glm::vec3(distance * 0.6f, distance * 0.4f, distance * 0.6f);

    glm::mat4 view = glm::lookAt(cameraPos, center, glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, distance * 10.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    // Shader simple inline
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 mvp;
        out vec3 Normal;
        
        void main() {
            gl_Position = mvp * vec4(aPos, 1.0);
            Normal = aNormal;
        }
    )";

    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 Normal;
        out vec4 FragColor;
        
        void main() {
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
            vec3 norm = normalize(Normal);
            float diff = max(dot(norm, lightDir), 0.0);
            
            vec3 baseColor = vec3(0.7, 0.7, 0.75);
            vec3 ambient = 0.3 * baseColor;
            vec3 diffuse = diff * baseColor;
            
            FragColor = vec4(ambient + diffuse, 1.0);
        }
    )";

    // Compilar shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Usar shader
    glUseProgram(shaderProgram);

    GLint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    if (mvpLoc != -1)
    {
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    }

    // Renderizar todas las meshes
    for (const Mesh* mesh : meshes)
    {
        if (mesh->VAO != 0 && !mesh->indices.empty())
        {
            glBindVertexArray(mesh->VAO);
            glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    // Limpiar shader
    glDeleteProgram(shaderProgram);

    // Restaurar estado
    glBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);

    // Limpiar framebuffer y depth buffer (pero mantener la textura)
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depthRBO);

    return colorTexture;
}