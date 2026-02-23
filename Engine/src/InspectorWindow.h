#pragma once

#include "EditorWindow.h"
#include <imgui.h>  
#include <ImGuizmo.h>
#include <vector>
#include "AudioSystem.h"
#include <unordered_map>
#include <nlohmann/json.hpp>

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
    bool DrawGameObjectSection(GameObject* selectedObject);
    void DrawGizmoSettings();
    void DrawTransformComponent(GameObject* selectedObject);
    void DrawCameraComponent(GameObject* selectedObject);
    void DrawMeshComponent(GameObject* selectedObject);
    void DrawMaterialComponent(GameObject* selectedObject);
    void DrawRotateComponent(GameObject* selectedObject);
    void DrawScriptComponent(GameObject* selectedObject);
    void DrawAddComponentButton(GameObject* selectedObject);
    
    void DrawParticleComponent(GameObject* selectedObject);
    void DrawRigidodyComponent(GameObject* selectedObject);
    void DrawBoxColliderComponent(GameObject* selectedObject);
    void DrawSphereColliderComponent(GameObject* selectedObject);
    void DrawCapsuleColliderComponent(GameObject* selectedObject);
    void DrawPlaneColliderComponent(GameObject* selectedObject);
    void DrawInfinitePlaneColliderComponent(GameObject* selectedObject);
    void DrawMeshColliderComponent(GameObject* selectedObject);
    void DrawConvexColliderComponent(GameObject* selectedObject);
    void DrawAudioSourceComponent(GameObject* selectedObject);
    void DrawAudioListenerComponent(GameObject* selectedObject);
    void DrawReverbZoneComponent(GameObject* selectedObject); 

    // Helper methods
    // Helper methods
    void GetAllGameObjects(GameObject* root, std::vector<GameObject*>& outObjects);
    bool IsDescendantOf(GameObject* potentialDescendant, GameObject* potentialAncestor);

    AudioSystem* audioSystem;

    //QOL
    bool m_WasAnyItemActive = false;
    std::unordered_map<Component*, nlohmann::json> m_ComponentSnapshots;

    // Gizmo state
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

    // Normals visualization
    bool showVertexNormals = false;
    bool showFaceNormals = false;

    Component* componentToRemove = nullptr;

};