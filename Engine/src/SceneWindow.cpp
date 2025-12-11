#include "SceneWindow.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "Application.h"
#include "GameObject.h"
#include "Transform.h"
#include "SelectionManager.h"
#include "InspectorWindow.h"
#include "Input.h"
#include <glm/glm.hpp>

SceneWindow::SceneWindow(InspectorWindow* inspector)
    : EditorWindow("Scene"), inspectorWindow(inspector)
{
}

void SceneWindow::Draw()
{
    if (!isOpen) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    sceneViewportPos = ImGui::GetCursorScreenPos();
    sceneViewportSize = ImGui::GetContentRegionAvail();

    GLuint sceneTexture = Application::GetInstance().renderer->GetSceneTexture();
    if (sceneTexture != 0 && sceneViewportSize.x > 0 && sceneViewportSize.y > 0)
    {
        ImTextureID texID = (ImTextureID)(uintptr_t)sceneTexture;
        ImGui::Image(texID, sceneViewportSize, ImVec2(0, 1), ImVec2(1, 0));
    }
    else
    {
        ImGui::InvisibleButton("SceneView", sceneViewportSize);
    }

    ImGuizmo::BeginFrame();

    HandleGizmoInput();
    DrawGizmo();

    ImGui::End();
    ImGui::PopStyleVar();
}

void SceneWindow::HandleGizmoInput()
{
    Input* input = Application::GetInstance().input.get();

    if (ImGui::GetIO().WantTextInput) return;

    if (input->GetKey(SDL_SCANCODE_W) == KEY_DOWN)
    {
        inspectorWindow->SetGizmoOperation(ImGuizmo::TRANSLATE);
    }

    if (input->GetKey(SDL_SCANCODE_E) == KEY_DOWN)
    {
        inspectorWindow->SetGizmoOperation(ImGuizmo::ROTATE);
    }

    if (input->GetKey(SDL_SCANCODE_R) == KEY_DOWN)
    {
        inspectorWindow->SetGizmoOperation(ImGuizmo::SCALE);
    }

    if (input->GetKey(SDL_SCANCODE_T) == KEY_DOWN)
    {
        ImGuizmo::MODE currentMode = inspectorWindow->GetCurrentGizmoMode();
        inspectorWindow->SetGizmoMode(
            (currentMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD
        );
    }

    if (input->GetKey(SDL_SCANCODE_Q) == KEY_DOWN)
    {
        inspectorWindow->SetGizmoOperation(ImGuizmo::BOUNDS);
    }
}

void SceneWindow::DrawGizmo()
{
    // Primero verificar si el gizmo estaba siendo usado en el frame anterior
    bool wasUsingGizmo = isGizmoActive;

    GameObject* selectedObject = Application::GetInstance().selectionManager->GetSelectedObject();
    if (!selectedObject)
    {
        isGizmoActive = false;

        // Si acabamos de soltar el gizmo, marcar que necesita rebuild
        if (wasUsingGizmo)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
        }
        return;
    }

    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera)
    {
        isGizmoActive = false;

        if (wasUsingGizmo)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
        }
        return;
    }

    Transform* transform = static_cast<Transform*>(selectedObject->GetComponent(ComponentType::TRANSFORM));
    if (!transform)
    {
        isGizmoActive = false;

        if (wasUsingGizmo)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
        }
        return;
    }

    if (sceneViewportSize.y <= 0.0f)
    {
        isGizmoActive = false;

        if (wasUsingGizmo)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
        }
        return;
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(sceneViewportPos.x, sceneViewportPos.y, sceneViewportSize.x, sceneViewportSize.y);

    float sceneAspect = sceneViewportSize.x / sceneViewportSize.y;
    float currentAspect = camera->GetAspectRatio();

    const float TOLERANCE = 0.001f;
    if (std::abs(currentAspect - sceneAspect) > TOLERANCE)
    {
        camera->SetAspectRatio(sceneAspect);
    }

    glm::mat4 viewMatrix = camera->GetViewMatrix();
    glm::mat4 projectionMatrix = camera->GetProjectionMatrix();
    glm::mat4 transformMatrix = transform->GetGlobalMatrix();

    ImGuizmo::OPERATION currentOp = inspectorWindow->GetCurrentGizmoOperation();
    ImGuizmo::MODE currentMode = inspectorWindow->GetCurrentGizmoMode();

    ImGuizmo::Manipulate(
        glm::value_ptr(viewMatrix),
        glm::value_ptr(projectionMatrix),
        currentOp,
        currentMode,
        glm::value_ptr(transformMatrix)
    );

    // Actualizar el flag de si el gizmo está siendo usado
    isGizmoActive = ImGuizmo::IsUsing();

    if (ImGuizmo::IsUsing())
    {
        GameObject* parent = selectedObject->GetParent();

        if (parent)
        {
            Transform* parentTransform = static_cast<Transform*>(parent->GetComponent(ComponentType::TRANSFORM));
            if (parentTransform)
            {
                const glm::mat4& parentGlobal = parentTransform->GetGlobalMatrix();
                transformMatrix = glm::inverse(parentGlobal) * transformMatrix;
            }
        }

        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        glm::decompose(transformMatrix, scale, rotation, position, skew, perspective);

        transform->SetPosition(position);
        transform->SetRotationQuat(rotation);
        transform->SetScale(scale);
    }
    else if (wasUsingGizmo)
    {
        // Acabamos de soltar el gizmo, marcar para rebuild del octree
        Application::GetInstance().scene->MarkOctreeForRebuild();
    }
}