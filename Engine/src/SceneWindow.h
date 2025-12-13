#pragma once

#include "EditorWindow.h"
#include <imgui.h>
#include <ImGuizmo.h>

class InspectorWindow;
class GameObject; 
struct aiNode; 

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
    void HandleAssetDropTarget();  
    unsigned long long FindTextureForDroppedMesh(unsigned long long meshUID); // Helper para encontrar textura
    void ApplyMeshTransformFromFBX(GameObject* meshObject, unsigned long long meshUID); // Helper para aplicar transformación

    InspectorWindow* inspectorWindow;

    ImVec2 sceneViewportPos;
    ImVec2 sceneViewportSize;

    bool isGizmoActive = false;
};