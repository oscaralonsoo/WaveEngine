#pragma once

#include "EditorWindow.h"

class GameObject;

class HierarchyWindow : public EditorWindow
{
public:
    HierarchyWindow();
    ~HierarchyWindow() override = default;

    void Draw() override;

private:
    void DrawGameObjectNode(GameObject* gameObject);
    void SelectGameObjectAndChildren(GameObject* gameObject);

    // Renaming functionality
    GameObject* renamingObject = nullptr;
    char renameBuffer[256] = "";
};