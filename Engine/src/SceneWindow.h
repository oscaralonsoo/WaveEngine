#pragma once

#include "EditorWindow.h"
#include <imgui.h>
#include <ImGuizmo.h>

class InspectorWindow;

class SceneWindow : public EditorWindow
{
public:
    SceneWindow(InspectorWindow* inspector);
    ~SceneWindow() override = default;

    void Draw() override;

    ImVec2 GetViewportPos() const { return sceneViewportPos; }
    ImVec2 GetViewportSize() const { return sceneViewportSize; }

    bool IsGizmoBeingUsed() const { return isGizmoActive; }

private:
    void HandleGizmoInput();
    void DrawGizmo();

    InspectorWindow* inspectorWindow;

    ImVec2 sceneViewportPos;
    ImVec2 sceneViewportSize;

    bool isGizmoActive = false;
};