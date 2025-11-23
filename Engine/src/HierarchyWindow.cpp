#include "HierarchyWindow.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include "Application.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "ModuleScene.h"
#include "Log.h"

HierarchyWindow::HierarchyWindow()
    : EditorWindow("Hierarchy")
{
}

void HierarchyWindow::Draw()
{
    if (!isOpen) return;

    ImGui::Begin(name.c_str(), &isOpen);

    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (root != nullptr)
    {
        DrawGameObjectNode(root);
    }
    else
    {
        ImGui::TextDisabled("No scene loaded");
    }

    ImGui::End();
}

void HierarchyWindow::DrawGameObjectNode(GameObject* gameObject)
{
    if (gameObject == nullptr)
        return;

    const std::vector<GameObject*>& children = gameObject->GetChildren();
    bool hasChildren = !children.empty();

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;

    SelectionManager* selectionManager = Application::GetInstance().selectionManager;
    if (selectionManager->IsSelected(gameObject))
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!hasChildren)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool isRenaming = (renamingObject == gameObject);

    if (isRenaming)
    {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##", renameBuffer, sizeof(renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            if (strlen(renameBuffer) > 0)
            {
                gameObject->SetName(renameBuffer);
                LOG_DEBUG("GameObject renamed to: %s", renameBuffer);
            }
            renamingObject = nullptr;
        }

        if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Escape)))
        {
            renamingObject = nullptr;
        }
    }
    else
    {
        // Checkbox for active state
        bool isActive = gameObject->IsActive();
        ImGui::PushID(gameObject);
        if (ImGui::Checkbox("##active", &isActive))
        {
            gameObject->SetActive(isActive);
        }
        ImGui::PopID();

        ImGui::SameLine();

        // Display the node
        bool nodeOpen = ImGui::TreeNodeEx(gameObject, nodeFlags, "%s", gameObject->GetName().c_str());

        // Handle selection
        if (ImGui::IsItemClicked())
        {
            const bool* keys = SDL_GetKeyboardState(NULL);
            bool shiftPressed = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

            if (shiftPressed)
            {
                selectionManager->ToggleSelection(gameObject);
            }
            else
            {
                if (hasChildren)
                {
                    selectionManager->ClearSelection();
                    SelectGameObjectAndChildren(gameObject);
                }
                else
                {
                    selectionManager->SetSelectedObject(gameObject);
                }
            }
        }

        // Handle renaming on double click
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            renamingObject = gameObject;
            strncpy(renameBuffer, gameObject->GetName().c_str(), sizeof(renameBuffer) - 1);
        }

        // Recursively draw children
        if (nodeOpen && hasChildren)
        {
            for (GameObject* child : children)
            {
                DrawGameObjectNode(child);
            }
            ImGui::TreePop();
        }
    }
}

void HierarchyWindow::SelectGameObjectAndChildren(GameObject* gameObject)
{
    if (gameObject == nullptr)
        return;

    SelectionManager* selectionManager = Application::GetInstance().selectionManager;
    selectionManager->AddToSelection(gameObject);

    const std::vector<GameObject*>& children = gameObject->GetChildren();
    for (GameObject* child : children)
    {
        SelectGameObjectAndChildren(child);
    }
}