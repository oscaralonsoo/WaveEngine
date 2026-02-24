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

    ImGuizmo::OPERATION GetCurrentGizmoOperation() const { return currentGizmoOperation; }
    ImGuizmo::MODE GetCurrentGizmoMode() const { return currentGizmoMode; }
    void SetGizmoOperation(ImGuizmo::OPERATION op) { currentGizmoOperation = op; }
    void SetGizmoMode(ImGuizmo::MODE mode) { currentGizmoMode = mode; }
    bool ShouldShowVertexNormals() const { return showVertexNormals; }
    bool ShouldShowFaceNormals() const { return showFaceNormals; }

private:
    bool DrawGameObjectSection(GameObject* selectedObject);
    void DrawGizmoSettings();
    void DrawAddComponentButton(GameObject* selectedObject);
    void DrawComponentContextMenu(Component* component, bool canRemove = true); // ahora método de instancia
    void DrawTransformComponent(GameObject* selectedObject);
    void DrawCameraComponent(GameObject* selectedObject);
    void DrawMeshComponent(GameObject* selectedObject);
    void DrawMaterialComponent(GameObject* selectedObject);
    void DrawRotateComponent(GameObject* selectedObject);
    void DrawScriptComponent(GameObject* selectedObject);
    void DrawParticleComponent(GameObject* selectedObject);
    void DrawRigidodyComponent(GameObject* selectedObject);
    void DrawBoxColliderComponent(GameObject* selectedObject);
    void DrawSphereColliderComponent(GameObject* selectedObject);
    void DrawCapsuleColliderComponent(GameObject* selectedObject);
    void DrawPlaneColliderComponent(GameObject* selectedObject);
    void DrawInfinitePlaneColliderComponent(GameObject* selectedObject);
    void DrawMeshColliderComponent(GameObject* selectedObject);
    void DrawConvexColliderComponent(GameObject* selectedObject);
    void DrawFixedJointComponent(GameObject* selectedObject);
    void DrawDistanceJointComponent(GameObject* selectedObject);
    void DrawHingeJointComponent(GameObject* selectedObject);
    void DrawSphericalJointComponent(GameObject* selectedObject);
    void DrawPrismaticJointComponent(GameObject* selectedObject);
    void DrawD6JointComponent(GameObject* selectedObject);
    void DrawCanvasComponent(GameObject* selectedObject);
    void DrawAudioSourceComponent(GameObject* selectedObject);
    void DrawAudioListenerComponent(GameObject* selectedObject);
    void DrawReverbZoneComponent(GameObject* selectedObject);

    void GetAllGameObjects(GameObject* root, std::vector<GameObject*>& outObjects);
    bool IsDescendantOf(GameObject* potentialDescendant, GameObject* potentialAncestor);

    bool IsPendingRemoval(Component* comp) const;

    AudioSystem* audioSystem;

    bool m_WasAnyItemActive = false;
    std::unordered_map<Component*, nlohmann::json> m_ComponentSnapshots;
    std::vector<Component*> m_PendingRemoval;

    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

    bool showVertexNormals = false;
    bool showFaceNormals = false;
};