#pragma once

#include "EditorWindow.h"
#include <imgui.h> 
#include <ImGuizmo.h>

class GameObject;

class InspectorWindow : public EditorWindow
{
public:
    InspectorWindow();
    ~InspectorWindow() override = default;

    void Draw() override;

    // Gizmo management
    ImGuizmo::OPERATION GetCurrentGizmoOperation() const { return currentGizmoOperation; }
    ImGuizmo::MODE GetCurrentGizmoMode() const { return currentGizmoMode; }

    void SetGizmoOperation(ImGuizmo::OPERATION op) { currentGizmoOperation = op; }
    void SetGizmoMode(ImGuizmo::MODE mode) { currentGizmoMode = mode; }

    bool ShouldShowVertexNormals() const { return showVertexNormals; }
    bool ShouldShowFaceNormals() const { return showFaceNormals; }

private:
    void DrawGizmoSettings();
    void DrawTransformComponent(GameObject* selectedObject);
    void DrawCameraComponent(GameObject* selectedObject);
    void DrawMeshComponent(GameObject* selectedObject);
    void DrawMaterialComponent(GameObject* selectedObject);

    // Gizmo state
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

    // Normals visualization
    bool showVertexNormals = false;
    bool showFaceNormals = false;
};