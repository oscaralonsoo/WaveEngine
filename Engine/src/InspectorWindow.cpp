#include "InspectorWindow.h"
#include <imgui.h>
#include "Application.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentRotate.h"
#include "Log.h"

InspectorWindow::InspectorWindow()
    : EditorWindow("Inspector")
{
}

void InspectorWindow::Draw()
{
    if (!isOpen) return;

    ImGui::Begin(name.c_str(), &isOpen);

    isHovered = (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow | ImGuiHoveredFlags_ChildWindows));

    SelectionManager* selectionManager = Application::GetInstance().selectionManager;

    if (!selectionManager->HasSelection())
    {
        ImGui::TextDisabled("No GameObject selected");
        ImGui::End();
        return;
    }

    GameObject* selectedObject = selectionManager->GetSelectedObject();

    if (selectedObject == nullptr)
    {
        ImGui::TextDisabled("Invalid selection");
        ImGui::End();
        return;
    }

    ImGui::Text("GameObject: %s", selectedObject->GetName().c_str());
    ImGui::SameLine();

    if (selectedObject->GetComponent(ComponentType::CAMERA)) {
        bool isActive = false; // ActiveCamera
        ImGui::Checkbox("##isActive",&isActive);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set has Active Camera");
        if (isActive) {
            ComponentCamera* cameraGo = Application::GetInstance().camera.get()->GetSceneCamera();
            ComponentCamera* ActiveCamera = Application::GetInstance().camera.get()->GetActiveCamera();
            if (cameraGo != ActiveCamera) {
                Application::GetInstance().camera->SetSceneCamera(cameraGo);
            }

        }
    }

    DrawGizmoSettings();
    ImGui::Separator();

    DrawTransformComponent(selectedObject);
    DrawCameraComponent(selectedObject);
    DrawMeshComponent(selectedObject);
    DrawMaterialComponent(selectedObject);
    DrawRotateComponent(selectedObject);

    ImGui::End();
}

void InspectorWindow::DrawGizmoSettings()
{
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Gizmo Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Gizmo Mode:");
        ImGui::SameLine();
        ImGui::Text("Turn off (Q)");

        bool isTranslate = (currentGizmoOperation == ImGuizmo::TRANSLATE);
        bool isRotate = (currentGizmoOperation == ImGuizmo::ROTATE);
        bool isScale = (currentGizmoOperation == ImGuizmo::SCALE);

        if (ImGui::RadioButton("Translate (W)", isTranslate))
        {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Move the object in 3D space\nShortcut: W key");

        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate (E)", isRotate))
        {
            currentGizmoOperation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Rotate the object\nShortcut: E key");

        ImGui::SameLine();
        if (ImGui::RadioButton("Scale (R)", isScale))
        {
            currentGizmoOperation = ImGuizmo::SCALE;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Scale the object\nShortcut: R key");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Transform Space (T):");
        ImGui::Spacing();

        bool isWorld = (currentGizmoMode == ImGuizmo::WORLD);
        bool isLocal = (currentGizmoMode == ImGuizmo::LOCAL);

        if (ImGui::RadioButton("World Space", isWorld))
        {
            currentGizmoMode = ImGuizmo::WORLD;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "World Space");
            ImGui::Separator();
            ImGui::Text("Transformations are relative to world axes");
            ImGui::BulletText("X: Always points right");
            ImGui::BulletText("Y: Always points up");
            ImGui::BulletText("Z: Always points forward");
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        if (ImGui::RadioButton("Local Space", isLocal))
        {
            currentGizmoMode = ImGuizmo::LOCAL;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Local Space");
            ImGui::Separator();
            ImGui::Text("Transformations are relative to objects rotation");
            ImGui::BulletText("Axes follow objects orientation");
            ImGui::BulletText("Useful for moving along objects direction");
            ImGui::EndTooltip();
        }
    }
}

void InspectorWindow::DrawTransformComponent(GameObject* selectedObject)
{
    Transform* transform = static_cast<Transform*>(selectedObject->GetComponent(ComponentType::TRANSFORM));

    if (transform == nullptr) return;

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        glm::vec3 position = transform->GetPosition();
        glm::vec3 rotation = transform->GetRotation();
        glm::vec3 scale = transform->GetScale();

        bool transformChanged = false;

        ImGui::Text("Position");
        ImGui::PushItemWidth(80);
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##PosX", &position.x, 0.1f)) transformChanged = true;
        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##PosY", &position.y, 0.1f)) transformChanged = true;
        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##PosZ", &position.z, 0.1f)) transformChanged = true;

        ImGui::Spacing();

        ImGui::Text("Rotation");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##RotX", &rotation.x, 0.1f)) transformChanged = true;
        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##RotY", &rotation.y, 0.1f)) transformChanged = true;
        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##RotZ", &rotation.z, 0.1f)) transformChanged = true;

        ImGui::Spacing();

        ImGui::Text("Scale");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##ScaleX", &scale.x, 0.1f)) transformChanged = true;
        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##ScaleY", &scale.y, 0.1f)) transformChanged = true;
        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##ScaleZ", &scale.z, 0.1f)) transformChanged = true;

        ImGui::PopItemWidth();

        if (transformChanged)
        {
            transform->SetPosition(position);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }

        ImGui::Spacing();

        if (ImGui::Button("Reset Transform"))
        {
            transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

            LOG_DEBUG("Transform reset for: %s", selectedObject->GetName().c_str());
            LOG_CONSOLE("Transform reset for: %s", selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Reset position to (0,0,0), rotation to (0,0,0), and scale to (1,1,1)");
        }
    }
}

void InspectorWindow::DrawCameraComponent(GameObject* selectedObject)
{
    ComponentCamera* cameraComp = static_cast<ComponentCamera*>(selectedObject->GetComponent(ComponentType::CAMERA));

    if (cameraComp == nullptr) return;

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Camera Settings");
        ImGui::Separator();

        float fov = cameraComp->GetFov();
        if (ImGui::SliderFloat("Field of View", &fov, 20.0f, 120.0f, "%.1f"))
        {
            cameraComp->SetFov(fov);
            LOG_DEBUG("Camera FOV set to: %.1f", fov);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Camera field of view in degrees");

        ImGui::Spacing();

        float nearPlane = cameraComp->GetNearPlane();
        float farPlane = cameraComp->GetFarPlane();

        if (ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, 10.0f, "%.2f"))
        {
            cameraComp->SetNearPlane(nearPlane);
            LOG_DEBUG("Camera near plane set to: %.2f", nearPlane);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Near clipping plane distance");

        if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, 10.0f, 1000.0f, "%.1f"))
        {
            cameraComp->SetFarPlane(farPlane);
            LOG_DEBUG("Camera far plane set to: %.1f", farPlane);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Far clipping plane distance");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Text("Optimization Settings");

        bool frustumEnabled = cameraComp->IsFrustumCullingEnabled();
        if (ImGui::Checkbox("Enable Frustum Culling", &frustumEnabled))
        {
            cameraComp->SetFrustumCulling(frustumEnabled);
            LOG_DEBUG("Frustum culling %s for camera: %s",
                frustumEnabled ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
            LOG_CONSOLE("Frustum culling %s for %s",
                frustumEnabled ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Frustum Culling");
            ImGui::Separator();
            ImGui::Text("When enabled, objects outside the camera's");
            ImGui::Text("view frustum will not be rendered.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Benefits:");
            ImGui::BulletText("Better performance in large scenes");
            ImGui::BulletText("Reduces GPU workload");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Status:");
            ImGui::BulletText(frustumEnabled ? "Objects outside view are HIDDEN" : "All objects are RENDERED");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        bool drawFrustum = cameraComp->ShouldDrawFrustum();
        if (ImGui::Checkbox("Draw Frustum Gizmo", &drawFrustum))
        {
            cameraComp->SetDrawFrustum(drawFrustum);
            LOG_DEBUG("Frustum visualization %s for camera: %s",
                drawFrustum ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
            LOG_CONSOLE("Frustum gizmo %s for %s",
                drawFrustum ? "enabled" : "disabled",
                selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Frustum Visualization");
            ImGui::Separator();
            ImGui::Text("Shows the camera's view frustum as a wireframe");
            ImGui::Text("in the 3D viewport.");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Color Coding:");
            ImGui::BulletText("Green: Active camera");
            ImGui::BulletText("Yellow: Inactive cameras");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Tip:");
            ImGui::Text("Use this to debug what each camera sees");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        if (frustumEnabled)
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "? Culling Active");
            if (drawFrustum)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "| ?? Frustum Visible");
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "? Culling Disabled");
            if (drawFrustum)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "| ?? Frustum Visible");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
    }
}

void InspectorWindow::DrawMeshComponent(GameObject* selectedObject)
{
    ComponentMesh* meshComp = static_cast<ComponentMesh*>(selectedObject->GetComponent(ComponentType::MESH));

    if (meshComp == nullptr || !meshComp->HasMesh()) return;

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const Mesh& mesh = meshComp->GetMesh();

        ImGui::Text("Vertices: %d", (int)mesh.vertices.size());
        ImGui::Text("Indices: %d", (int)mesh.indices.size());
        ImGui::Text("Triangles: %d", (int)mesh.indices.size() / 3);

        ImGui::Separator();

        ImGui::Text("Normals Visualization:");

        if (ImGui::Checkbox("Show Vertex Normals", &showVertexNormals))
        {
            LOG_DEBUG("Vertex normals visualization: %s", showVertexNormals ? "ON" : "OFF");
        }

        if (ImGui::Checkbox("Show Face Normals", &showFaceNormals))
        {
            LOG_DEBUG("Face normals visualization: %s", showFaceNormals ? "ON" : "OFF");
        }
    }
}

void InspectorWindow::DrawMaterialComponent(GameObject* selectedObject)
{
    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(selectedObject->GetComponent(ComponentType::MATERIAL));

    if (materialComp != nullptr)
    {
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Texture Information:");

            std::string texturePath = materialComp->GetTexturePath();
            if (texturePath.empty() || texturePath == "[Checkerboard Pattern]")
            {
                ImGui::Text("Path: Checkerboard");
            }
            else
            {
                ImGui::Text("Path: %s", texturePath.c_str());
            }

            ImGui::Text("Size: %d x %d pixels", materialComp->GetTextureWidth(), materialComp->GetTextureHeight());

            ImGui::Separator();

            if (materialComp->HasOriginalTexture())
            {
                ImGui::Separator();
                ImGui::Text("Original Texture:");
                ImGui::Text("Path: %s", materialComp->GetOriginalTexturePath().c_str());
            }

            ImGui::Separator();

            ImGui::BeginGroup();

            if (ImGui::Button("Apply Checkerboard"))
            {
                materialComp->CreateCheckerboardTexture();
                LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Applies the default black and white checkerboard pattern to this GameObject");
            }

            if (materialComp->HasOriginalTexture())
            {
                ImGui::SameLine();

                if (ImGui::Button("Restore Original"))
                {
                    materialComp->RestoreOriginalTexture();
                    LOG_DEBUG("Restored original texture to: %s", selectedObject->GetName().c_str());
                    LOG_CONSOLE("Original texture restored to %s", selectedObject->GetName().c_str());
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Restores the original texture that was previously loaded");
                }
            }

            ImGui::EndGroup();
        }
    }
    else
    {
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(selectedObject->GetComponent(ComponentType::MESH));
        if (meshComp != nullptr && meshComp->HasMesh())
        {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("No material component");
            }
        }
    }
}

void InspectorWindow::DrawRotateComponent(GameObject* selectedObject)
{
    ComponentRotate* rotateComp = static_cast<ComponentRotate*>(selectedObject->GetComponent(ComponentType::ROTATE));

    if (rotateComp == nullptr) return;

    if (ImGui::CollapsingHeader("Auto Rotate", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool active = rotateComp->IsActive();
        if (ImGui::Checkbox("Enable Auto Rotation", &active))
        {
            rotateComp->SetActive(active);
        }

        rotateComp->OnEditor();
    }
}