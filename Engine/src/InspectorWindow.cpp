#include "InspectorWindow.h"
#include <imgui.h>
#include "Application.h"
#include "GameObject.h"
#include "SelectionManager.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include "ComponentRotate.h"
#include "ResourceTexture.h"
#include "ComponentParticleSystem.h"
#include "Rigidbody.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include "MeshCollider.h"
#include "ConvexCollider.h"
#include "PlaneCollider.h"
#include "InfinitePlaneCollider.h"
#include "Log.h"
#include "ComponentScript.h"
#include "ComponentNavigation.h"
#include "NavMeshManager.h"
#include <filesystem>

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

    if (selectedObject == nullptr || selectedObject->IsMarkedForDeletion())
    {
        ImGui::TextDisabled("Invalid selection");
        ImGui::End();
        return;
    }

    ImGui::Text("GameObject: %s", selectedObject->GetName().c_str());
    ImGui::SameLine();

    if (selectedObject->GetComponent(ComponentType::CAMERA)) {
        ComponentCamera* selectedCamera = static_cast<ComponentCamera*>(selectedObject->GetComponent(ComponentType::CAMERA));
        ComponentCamera* currentSceneCamera = Application::GetInstance().camera->GetSceneCamera();

        bool isActive = (selectedCamera == currentSceneCamera);

        if (ImGui::Checkbox("##isActive", &isActive)) {
            if (isActive) {
                Application::GetInstance().camera->SetSceneCamera(selectedCamera);
            }
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Set as Active Game Camera");
        }
    }

    ImGui::Separator();

    bool objectDeleted = DrawGameObjectSection(selectedObject);
    if (objectDeleted)
    {
        ImGui::End();
        return;
    }

    ImGui::Separator();
    DrawGizmoSettings();
    ImGui::Separator();

    DrawTransformComponent(selectedObject);
    DrawCameraComponent(selectedObject);
    DrawMeshComponent(selectedObject);
    DrawMaterialComponent(selectedObject);
    DrawRotateComponent(selectedObject);
    DrawScriptComponent(selectedObject); 
    DrawParticleComponent(selectedObject);
    DrawRigidodyComponent(selectedObject);
    DrawBoxColliderComponent(selectedObject);
    DrawSphereColliderComponent(selectedObject);
    DrawCapsuleColliderComponent(selectedObject);
    DrawPlaneColliderComponent(selectedObject);
    DrawInfinitePlaneColliderComponent(selectedObject);
    DrawMeshColliderComponent(selectedObject);
    DrawConvexColliderComponent(selectedObject);
    DrawNavigationSection(selectedObject);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawAddComponentButton(selectedObject);

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
        bool wasEditing = false;

        ImGui::Text("Position");
        ImGui::PushItemWidth(80);
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##PosX", &position.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##PosY", &position.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##PosZ", &position.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::Spacing();

        ImGui::Text("Rotation");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##RotX", &rotation.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##RotY", &rotation.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##RotZ", &rotation.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::Spacing();

        ImGui::Text("Scale");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##ScaleX", &scale.x, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##ScaleY", &scale.y, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##ScaleZ", &scale.z, 0.1f)) transformChanged = true;
        if (ImGui::IsItemDeactivatedAfterEdit()) wasEditing = true;

        ImGui::PopItemWidth();

        if (transformChanged)
        {
            transform->SetPosition(position);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }

        if (wasEditing)
        {
            Application::GetInstance().scene->MarkOctreeForRebuild();
            LOG_DEBUG("Octree rebuild requested after editing transform");
        }

        ImGui::Spacing();

        if (ImGui::Button("Reset Transform"))
        {
            transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
            transform->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

            // Rebuild después de reset
            Application::GetInstance().scene->MarkOctreeForRebuild();

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

    if (meshComp == nullptr) return;

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Mesh:");
        ImGui::SameLine();

        // Mesh display
        std::string currentMeshName = "None";
        if (meshComp->HasMesh() && meshComp->IsUsingResourceMesh())
        {
            UID currentUID = meshComp->GetMeshUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res)
            {
                currentMeshName = std::string(res->GetAssetFile());
                // Filename
                size_t lastSlash = currentMeshName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    currentMeshName = currentMeshName.substr(lastSlash + 1);
            }
            else
            {
                //Show UID
                currentMeshName = "UID " + std::to_string(currentUID);
            }
        }
        else if (meshComp->HasMesh() && meshComp->IsUsingDirectMesh())
        {
            currentMeshName = "[Primitive]";
        }

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##MeshSelector", currentMeshName.c_str()))
        {
			// Get mesh resources
            ModuleResources* resources = Application::GetInstance().resources.get();
            const std::map<UID, Resource*>& allResources = resources->GetAllResources();

            for (const auto& pair : allResources)
            {
                const Resource* res = pair.second;
                if (res->GetType() == Resource::MESH)
                {
                    std::string meshName = res->GetAssetFile();

                    // Filename
                    size_t lastSlash = meshName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) meshName = meshName.substr(lastSlash + 1);

                    UID meshUID = res->GetUID();
                    bool isSelected = (meshComp->IsUsingResourceMesh() && meshComp->GetMeshUID() == meshUID);

                    std::string displayName = meshName;
                    if (res->IsLoadedToMemory())
                    {
                        displayName += " [Loaded]";
                    }

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        if (meshComp->LoadMeshByUID(meshUID))
                        {
                            LOG_DEBUG("Assigned mesh '%s' (UID %llu) to GameObject '%s'",
                                meshName.c_str(), meshUID, selectedObject->GetName().c_str());
                            LOG_CONSOLE("Mesh '%s' assigned to '%s'",
                                meshName.c_str(), selectedObject->GetName().c_str());
                        }
                        else
                        {
                            LOG_CONSOLE("Failed to load mesh '%s' (UID %llu)", meshName.c_str(), meshUID);
                        }
                    }

                    if (isSelected)
                    {
						ImGui::SetItemDefaultFocus(); // Highlight selected item
                    }

                    // Show tooltip with UID and path
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("UID: %llu", meshUID);
                        ImGui::Text("Path: %s", res->GetAssetFile());
                        if (res->IsLoadedToMemory())
                        {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                        }
                        ImGui::EndTooltip();
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (meshComp->HasMesh())
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            const Mesh& mesh = meshComp->GetMesh();

            ImGui::Text("Mesh Statistics:");
            ImGui::Text("Vertices: %d", (int)mesh.vertices.size());
            ImGui::Text("Indices: %d", (int)mesh.indices.size());
            ImGui::Text("Triangles: %d", (int)mesh.indices.size() / 3);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

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
}

void InspectorWindow::DrawMaterialComponent(GameObject* selectedObject)
{
    ComponentMaterial* materialComp = static_cast<ComponentMaterial*>(selectedObject->GetComponent(ComponentType::MATERIAL));

    if (materialComp == nullptr) return;

    materialComp->OnEditor();
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

void InspectorWindow::DrawParticleComponent(GameObject* selectedObject)
{
    ComponentParticleSystem* particleComp = static_cast<ComponentParticleSystem*>(selectedObject->GetComponent(ComponentType::PARTICLE));

    if (particleComp != nullptr)
    {
        // Delegate the ui to the component
        particleComp->OnEditor();
    }
}

void  InspectorWindow::DrawRigidodyComponent(GameObject* selectedObject)
{
    Rigidbody* rigidbody = static_cast<Rigidbody*>(selectedObject->GetComponent(ComponentType::RIGIDBODY));

    if (rigidbody != nullptr)
    {
        // Delegate the ui to the component
        rigidbody->OnEditor();
    }
}

void  InspectorWindow::DrawBoxColliderComponent(GameObject* selectedObject)
{
    BoxCollider* boxCollider = static_cast<BoxCollider*>(selectedObject->GetComponent(ComponentType::BOX_COLLIDER));

    if (boxCollider != nullptr)
    {
        // Delegate the ui to the component
        boxCollider->OnEditor();
    }
}

void  InspectorWindow::DrawSphereColliderComponent(GameObject* selectedObject)
{
    SphereCollider* Collider = static_cast<SphereCollider*>(selectedObject->GetComponent(ComponentType::SPHERE_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}

void  InspectorWindow::DrawCapsuleColliderComponent(GameObject* selectedObject)
{
    CapsuleCollider* Collider = static_cast<CapsuleCollider*>(selectedObject->GetComponent(ComponentType::CAPSULE_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}

void  InspectorWindow::DrawPlaneColliderComponent(GameObject* selectedObject)
{
    PlaneCollider* Collider = static_cast<PlaneCollider*>(selectedObject->GetComponent(ComponentType::PLANE_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}
void  InspectorWindow::DrawInfinitePlaneColliderComponent(GameObject* selectedObject)
{
    InfinitePlaneCollider* Collider = static_cast<InfinitePlaneCollider*>(selectedObject->GetComponent(ComponentType::INFINITE_PLANE_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}
void  InspectorWindow::DrawMeshColliderComponent(GameObject* selectedObject)
{
    MeshCollider* Collider = static_cast<MeshCollider*>(selectedObject->GetComponent(ComponentType::MESH_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}
void  InspectorWindow::DrawConvexColliderComponent(GameObject* selectedObject)
{
    ConvexCollider* Collider = static_cast<ConvexCollider*>(selectedObject->GetComponent(ComponentType::CONVEX_COLLIDER));

    if (Collider != nullptr)
    {
        // Delegate the ui to the component
        Collider->OnEditor();
    }
}

void InspectorWindow::DrawNavigationSection(GameObject* selectedObject)
{
    ComponentNavigation* navComp = static_cast<ComponentNavigation*>(selectedObject->GetComponent(ComponentType::NAVIGATION));

    if (navComp != nullptr)
    {
        navComp->OnEditor();
    }
}

bool InspectorWindow::DrawGameObjectSection(GameObject* selectedObject)
{
    bool objectDeleted = false;

    if (ImGui::CollapsingHeader("GameObject", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Actions:");
        ImGui::Spacing();

        // Delete button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));

        if (ImGui::Button("Delete GameObject", ImVec2(-1, 0)))
        {
            if (selectedObject != Application::GetInstance().scene->GetRoot())
            {
                selectedObject->MarkForDeletion();
                LOG_DEBUG("GameObject '%s' marked for deletion", selectedObject->GetName().c_str());
                LOG_CONSOLE("GameObject '%s' marked for deletion", selectedObject->GetName().c_str());

                Application::GetInstance().selectionManager->ClearSelection();
                objectDeleted = true;
            }
            else
            {
                LOG_CONSOLE("Cannot delete Root GameObject!");
            }
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Delete GameObject");
            ImGui::Separator();
            ImGui::Text("Marks this GameObject for deletion");
            ImGui::Text("Shortcut: Backspace key");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        // Create empty child button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));

        if (ImGui::Button("Create Empty Child", ImVec2(-1, 0)))
        {
            GameObject* newChild = Application::GetInstance().scene->CreateGameObject("Empty");
            newChild->SetParent(selectedObject);

            Application::GetInstance().selectionManager->SetSelectedObject(newChild);

            LOG_DEBUG("Created empty child for '%s'", selectedObject->GetName().c_str());
            LOG_CONSOLE("Created empty child '%s' under '%s'", newChild->GetName().c_str(), selectedObject->GetName().c_str());
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Create Empty Child");
            ImGui::Separator();
            ImGui::Text("Creates a new empty GameObject as a child");
            ImGui::Text("of this GameObject");
            ImGui::EndTooltip();
        }
    }

    return objectDeleted;
}

void InspectorWindow::GetAllGameObjects(GameObject* root, std::vector<GameObject*>& outObjects)
{
    if (root == nullptr)
        return;

    outObjects.push_back(root);

    const std::vector<GameObject*>& children = root->GetChildren();
    for (GameObject* child : children)
    {
        GetAllGameObjects(child, outObjects);
    }
}

bool InspectorWindow::IsDescendantOf(GameObject* potentialDescendant, GameObject* potentialAncestor)
{
    if (potentialDescendant == nullptr || potentialAncestor == nullptr)
        return false;

    GameObject* current = potentialDescendant->GetParent();
    while (current != nullptr)
    {
        if (current == potentialAncestor)
            return true;
        current = current->GetParent();
    }

    return false;
}

void InspectorWindow::DrawScriptComponent(GameObject* selectedObject)
{
    ComponentScript* scriptComp = static_cast<ComponentScript*>(
        selectedObject->GetComponent(ComponentType::SCRIPT)
        );

    if (scriptComp == nullptr) return;

    if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent();

        if (scriptComp->HasScript()) {
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(scriptComp->GetScriptUID());

            if (res) {
                std::string filename = std::filesystem::path(res->GetAssetFile()).filename().string();

                ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Current Script:");
                ImGui::SameLine();
                ImGui::Text("%s", filename.c_str());

                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "UID: %llu", scriptComp->GetScriptUID());

                ImGui::Spacing();

                if (ImGui::Button("Change Script", ImVec2(120, 0))) {
                    ImGui::OpenPopup("SelectScript");
                }

                ImGui::SameLine();

                if (ImGui::Button("Remove Script", ImVec2(120, 0))) {
                    scriptComp->UnloadScript();
                    LOG_CONSOLE("[Inspector] Script removed from: %s", selectedObject->GetName().c_str());
                }

                ImGui::SameLine();

                if (ImGui::Button("Reload", ImVec2(80, 0))) {
                    scriptComp->ReloadScript();
                    LOG_CONSOLE("[Inspector] Script reloaded for: %s", selectedObject->GetName().c_str());
                }

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Hot reload the script");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                const auto& publicVars = scriptComp->GetPublicVariables();

                if (!publicVars.empty()) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Public Variables");
                    ImGui::Spacing();

                    for (size_t i = 0; i < publicVars.size(); ++i) {
                        const ScriptVariable& var = publicVars[i];

                        ImGui::PushID(i);

                        switch (var.type) {
                        case ScriptVarType::NUMBER: {
                            float value = std::get<float>(var.value);
                            if (ImGui::DragFloat(var.name.c_str(), &value, 0.1f)) {
                                ScriptVariable newVar(var.name, value);
                                scriptComp->UpdatePublicVariable(i, newVar);
                            }
                            break;
                        }

                        case ScriptVarType::STRING: {
                            static char buffer[256];
                            std::string value = std::get<std::string>(var.value);
                            strncpy(buffer, value.c_str(), sizeof(buffer) - 1);

                            if (ImGui::InputText(var.name.c_str(), buffer, sizeof(buffer))) {
                                ScriptVariable newVar(var.name, std::string(buffer));
                                scriptComp->UpdatePublicVariable(i, newVar);
                            }
                            break;
                        }

                        case ScriptVarType::BOOLEAN: {
                            bool value = std::get<bool>(var.value);
                            if (ImGui::Checkbox(var.name.c_str(), &value)) {
                                ScriptVariable newVar(var.name, value);
                                scriptComp->UpdatePublicVariable(i, newVar);
                            }
                            break;
                        }

                        case ScriptVarType::VEC3: {
                            glm::vec3 value = std::get<glm::vec3>(var.value);

                            ImGui::Text("%s", var.name.c_str());
                            ImGui::PushItemWidth(80);

                            bool changed = false;
                            ImGui::Text("X"); ImGui::SameLine(20);
                            if (ImGui::DragFloat("##X", &value.x, 0.1f)) changed = true;

                            ImGui::SameLine(120);
                            ImGui::Text("Y"); ImGui::SameLine(130);
                            if (ImGui::DragFloat("##Y", &value.y, 0.1f)) changed = true;

                            ImGui::SameLine(230);
                            ImGui::Text("Z"); ImGui::SameLine(240);
                            if (ImGui::DragFloat("##Z", &value.z, 0.1f)) changed = true;

                            ImGui::PopItemWidth();

                            if (changed) {
                                ScriptVariable newVar(var.name, value);
                                scriptComp->UpdatePublicVariable(i, newVar);
                            }
                            break;
                        }
                        }

                        ImGui::PopID();
                    }
                }
                else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                        "No public variables defined");
                    ImGui::Spacing();
                    ImGui::TextWrapped("Define variables in a 'public' table in your Lua script");
                }
            }
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No script assigned");

            if (ImGui::Button("Assign Script", ImVec2(150, 0))) {
                ImGui::OpenPopup("SelectScript");
            }
        }

        // Popup para seleccionar script
        if (ImGui::BeginPopup("SelectScript")) {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Select Lua Script");
            ImGui::Separator();

            ModuleResources* resources = Application::GetInstance().resources.get();
            const auto& allResources = resources->GetAllResources();

            bool foundScripts = false;

            for (const auto& [uid, resource] : allResources) {
                if (resource->GetType() == Resource::SCRIPT) {
                    foundScripts = true;

                    std::string filepath = resource->GetAssetFile();
                    std::string filename = std::filesystem::path(filepath).filename().string();

                    bool isSelected = (scriptComp->HasScript() && scriptComp->GetScriptUID() == uid);

                    if (ImGui::Selectable(filename.c_str(), isSelected)) {
                        if (scriptComp->LoadScriptByUID(uid)) {
                            LOG_CONSOLE("[Inspector] Script '%s' assigned to '%s'",
                                filename.c_str(), selectedObject->GetName().c_str());
                        }
                        else {
                            LOG_CONSOLE("[Inspector] Failed to load script '%s'", filename.c_str());
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("UID: %llu", uid);
                        ImGui::Text("Path: %s", filepath.c_str());
                        ImGui::EndTooltip();
                    }
                }
            }

            if (!foundScripts) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No .lua scripts found");
                ImGui::Spacing();
                ImGui::TextWrapped("Create a .lua file in Assets/Scripts/");
            }

            ImGui::EndPopup();
        }

        ImGui::Unindent();
    }
}

void InspectorWindow::DrawAddComponentButton(GameObject* selectedObject)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.7f, 1.0f));

    if (ImGui::Button("Add Component", ImVec2(-1, 30)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Add Component");
        ImGui::Separator();
        ImGui::Text("Add a new component to this GameObject");
        ImGui::EndTooltip();
    }

    // Popup con lista de componentes
    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Select Component Type");
        ImGui::Separator();
        ImGui::Spacing();

        // Script Component
        bool hasScript = (selectedObject->GetComponent(ComponentType::SCRIPT) != nullptr);

        if (hasScript)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Selectable("Script", false, hasScript ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::SCRIPT);  

            LOG_CONSOLE("[Inspector] Script component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasScript)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered() && !hasScript)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Lua script to this GameObject");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasScript)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Script component");
            ImGui::EndTooltip();
        }

        // Camera Component
        bool hasCamera = (selectedObject->GetComponent(ComponentType::CAMERA) != nullptr);

        if (hasCamera)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Selectable("Camera", false, hasCamera ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::CAMERA);  

            LOG_CONSOLE("[Inspector] Camera component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasCamera)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered() && !hasCamera)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Camera to this GameObject");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasCamera)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Camera component");
            ImGui::EndTooltip();
        }

        // Mesh Component
        bool hasMesh = (selectedObject->GetComponent(ComponentType::MESH) != nullptr);

        if (hasMesh)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Selectable("Mesh Renderer", false, hasMesh ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::MESH);  

            LOG_CONSOLE("[Inspector] Mesh component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasMesh)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered() && !hasMesh)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Mesh Renderer to this GameObject");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasMesh)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Mesh component");
            ImGui::EndTooltip();
        }

        // Material Component
        bool hasMaterial = (selectedObject->GetComponent(ComponentType::MATERIAL) != nullptr);

        if (hasMaterial)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Selectable("Material", false, hasMaterial ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::MATERIAL); 

            LOG_CONSOLE("[Inspector] Material component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasMaterial)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered() && !hasMaterial)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Material to this GameObject");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasMaterial)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Material component");
            ImGui::EndTooltip();
        }

        // Particle Component
        bool hasParticles = (selectedObject->GetComponent(ComponentType::PARTICLE) != nullptr);

        if (hasParticles) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        if (ImGui::Selectable("Particle System", false, hasParticles ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::PARTICLE);
            LOG_CONSOLE("[Inspector] Particle System component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasParticles) ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && !hasParticles)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Particle System");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasParticles)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Particle System");
            ImGui::EndTooltip();
        }

        bool hasRigidbody = (selectedObject->GetComponent(ComponentType::RIGIDBODY) != nullptr);
        
        if (hasRigidbody) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        if (ImGui::Selectable("Rigidbody", false, hasParticles ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::RIGIDBODY);
            LOG_CONSOLE("[Inspector] Rigidbody component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasRigidbody) ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && !hasRigidbody)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Rigidbody");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasRigidbody)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Rigidbody");
            ImGui::EndTooltip();
        }


        if (ImGui::Selectable("Box Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::BOX_COLLIDER);
            LOG_CONSOLE("[Inspector] Box Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Box Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Sphere Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::SPHERE_COLLIDER);
            LOG_CONSOLE("[Inspector] Sphere Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Sphere Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Capsule Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::CAPSULE_COLLIDER);
            LOG_CONSOLE("[Inspector] Capsule Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Capsule Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Plane Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::PLANE_COLLIDER);
            LOG_CONSOLE("[Inspector] Plane Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Plane Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Infinite Plane Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::INFINITE_PLANE_COLLIDER);
            LOG_CONSOLE("[Inspector] Infinite Plane Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Infinite Plane Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Mesh Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::MESH_COLLIDER);
            LOG_CONSOLE("[Inspector] Mesh Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Mesh Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Convex Collider", false))
        {
            selectedObject->CreateComponent(ComponentType::CONVEX_COLLIDER);
            LOG_CONSOLE("[Inspector] Convex Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Convex Collider");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Fixed Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::FIXED_JOINT);
            LOG_CONSOLE("[Inspector] Fixed Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Fixed Joint");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Distance Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::DISTANCE_JOINT);
            LOG_CONSOLE("[Inspector] Distance Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Distance Joint");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Hinge Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::HINGE_JOINT);
            LOG_CONSOLE("[Inspector] Hinge Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Hinge Joint");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Spherical Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::SPHERICAL_JOINT);
            LOG_CONSOLE("[Inspector] Spherical Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Spherical Joint");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("Prismatic Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::PRISMATIC_JOINT);
            LOG_CONSOLE("[Inspector] Prismatic Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Prismatic Joint");
            ImGui::EndTooltip();
        }
        
        if (ImGui::Selectable("D6 Joint", false))
        {
            selectedObject->CreateComponent(ComponentType::D6_JOINT);
            LOG_CONSOLE("[Inspector] D6 Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() )
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a D6 Joint");
            ImGui::EndTooltip();
        }

        // Navigation Component
        bool hasNavigation = (selectedObject->GetComponent(ComponentType::NAVIGATION) != nullptr);

        if (hasNavigation) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        if (ImGui::Selectable("Navigation & AI", false, hasNavigation ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::NAVIGATION);
            LOG_CONSOLE("[Inspector] Navigation component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasNavigation) ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            if (!hasNavigation) {
                ImGui::Text("Add a Navigation component for NavMesh baking");
            }
            else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Navigation component");
            }
            ImGui::EndTooltip();

        }

        ImGui::EndPopup();
    }
}