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
#include "ComponentCanvas.h"
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
#include "AudioComponent.h"
#include "AudioSource.h"
#include "AudioListener.h"
#include "ReverbZone.h"
#include "TransformCommand.h"
#include "ModuleEditor.h"
#include "ComponentCommand.h"
#include "Joint.h"
#include "FixedJoint.h"
#include "DistanceJoint.h"
#include "HingeJoint.h"
#include "SphericalJoint.h"
#include "PrismaticJoint.h"
#include "D6Joint.h"
#include "Log.h"
#include "ComponentScript.h"
#include <filesystem>
#include <nlohmann/json.hpp>

static nlohmann::json copiedComponentData;
static ComponentType copiedComponentType = static_cast<ComponentType>(-1);

static void DrawComponentContextMenu(Component* component, bool canRemove = true)
{
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Copy Component Values"))
        {
            copiedComponentData.clear();
            component->Serialize(copiedComponentData);
            copiedComponentType = component->GetType();
        }

        bool canPaste = (copiedComponentType == component->GetType() && !copiedComponentData.empty());
        if (ImGui::MenuItem("Paste Component Values", nullptr, false, canPaste))
        {
            component->Deserialize(copiedComponentData);
        }

        if (canRemove)
        {
            ImGui::Separator();
            if (ImGui::MenuItem("Remove Component"))
            {
                component->markedForRemoval = true;
            }
        }

        ImGui::EndPopup();
    }
}

InspectorWindow::InspectorWindow() : EditorWindow("Inspector"){ }

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
    DrawCanvasComponent(selectedObject);
    DrawAudioSourceComponent(selectedObject);
    DrawAudioListenerComponent(selectedObject);
    DrawReverbZoneComponent(selectedObject);
    DrawFixedJointComponent(selectedObject);
    DrawDistanceJointComponent(selectedObject);
    DrawHingeJointComponent(selectedObject);
    DrawSphericalJointComponent(selectedObject);
    DrawPrismaticJointComponent(selectedObject);
    DrawD6JointComponent(selectedObject);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawAddComponentButton(selectedObject);

    //handle components removal in inspector
    std::vector<Component*> componentsToRemove;
    for (Component* comp : selectedObject->GetComponents())
    {
        if (comp->markedForRemoval) componentsToRemove.push_back(comp);
    }
    for (Component* comp : componentsToRemove)
    {
        comp->markedForRemoval = false;
        Application::GetInstance().editor->GetCommandHistory()->ExecuteCommand(
            std::make_unique<RemoveComponentCommand>(selectedObject, comp)
        );
    }

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

    bool open = ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(transform, false);
    if (open)
    {
        glm::vec3 position = transform->GetPosition();
        glm::vec3 rotation = transform->GetRotation();
        glm::vec3 scale = transform->GetScale();

        static glm::vec3 posBeforeEdit, rotBeforeEdit, scaleBeforeEdit;
        static bool snapshotTaken = false;

        bool transformChanged = false;
        bool editFinished = false;

        auto CaptureSnapshotIfNeeded = [&]()
            {
                if (ImGui::IsItemActivated())
                {
                    posBeforeEdit = transform->GetPosition();
                    rotBeforeEdit = transform->GetRotation();
                    scaleBeforeEdit = transform->GetScale();
                    snapshotTaken = true;
                }
            };

        ImGui::Text("Position");
        ImGui::PushItemWidth(80);
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##PosX", &position.x, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##PosY", &position.y, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##PosZ", &position.z, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::Spacing();

        ImGui::Text("Rotation");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##RotX", &rotation.x, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##RotY", &rotation.y, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##RotZ", &rotation.z, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::Spacing();

        ImGui::Text("Scale");
        ImGui::Text("X"); ImGui::SameLine(20);
        if (ImGui::DragFloat("##ScaleX", &scale.x, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(120);
        ImGui::Text("Y"); ImGui::SameLine(130);
        if (ImGui::DragFloat("##ScaleY", &scale.y, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::SameLine(230);
        ImGui::Text("Z"); ImGui::SameLine(240);
        if (ImGui::DragFloat("##ScaleZ", &scale.z, 0.1f)) transformChanged = true;
        CaptureSnapshotIfNeeded();
        if (ImGui::IsItemDeactivatedAfterEdit()) editFinished = true;

        ImGui::PopItemWidth();

        if (transformChanged)
        {
            transform->SetPosition(position);
            transform->SetRotation(rotation);
            transform->SetScale(scale);
        }

        if (editFinished && snapshotTaken)
        {
            CommandHistory* history = Application::GetInstance().editor->GetCommandHistory();
            history->ExecuteCommand(std::make_unique<TransformCommand>(
                selectedObject,
                posBeforeEdit, rotBeforeEdit, scaleBeforeEdit,
                transform->GetPosition(), transform->GetRotation(), transform->GetScale()
            ));

            Application::GetInstance().scene->MarkOctreeForRebuild();
            LOG_DEBUG("Octree rebuild requested after editing transform");
            snapshotTaken = false;
        }

        ImGui::Spacing();

        if (ImGui::Button("Reset Transform"))
        {
            CommandHistory* history = Application::GetInstance().editor->GetCommandHistory();
            history->ExecuteCommand(std::make_unique<TransformCommand>(
                selectedObject,
                transform->GetPosition(), transform->GetRotation(), transform->GetScale(),
                glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)
            ));

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

    bool open = ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(cameraComp);
    if (open)
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

    bool open = ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(meshComp);
    if (open)
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

    bool open = ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(materialComp);
    if (open) materialComp->OnEditor();
}

void InspectorWindow::DrawRotateComponent(GameObject* selectedObject)
{
    ComponentRotate* rotateComp = static_cast<ComponentRotate*>(selectedObject->GetComponent(ComponentType::ROTATE));

    if (rotateComp == nullptr) return;

    bool open = ImGui::CollapsingHeader("Auto Rotate", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(rotateComp);
    if (open)
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
    if (particleComp == nullptr) return;

    bool open = ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(particleComp);
    if (open) particleComp->OnEditor();
}

void  InspectorWindow::DrawRigidodyComponent(GameObject* selectedObject)
{
    Rigidbody* rigidbody = static_cast<Rigidbody*>(selectedObject->GetComponent(ComponentType::RIGIDBODY));

    if (rigidbody != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(rigidbody);
        if (open) rigidbody->OnEditor();
    }
}

void  InspectorWindow::DrawBoxColliderComponent(GameObject* selectedObject)
{
    BoxCollider* boxCollider = static_cast<BoxCollider*>(selectedObject->GetComponent(ComponentType::BOX_COLLIDER));

    if (boxCollider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(boxCollider);
        if (open) boxCollider->OnEditor();
    }
}

void  InspectorWindow::DrawSphereColliderComponent(GameObject* selectedObject)
{
    SphereCollider* Collider = static_cast<SphereCollider*>(selectedObject->GetComponent(ComponentType::SPHERE_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

void  InspectorWindow::DrawCapsuleColliderComponent(GameObject* selectedObject)
{
    CapsuleCollider* Collider = static_cast<CapsuleCollider*>(selectedObject->GetComponent(ComponentType::CAPSULE_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Capsule Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

void  InspectorWindow::DrawPlaneColliderComponent(GameObject* selectedObject)
{
    PlaneCollider* Collider = static_cast<PlaneCollider*>(selectedObject->GetComponent(ComponentType::PLANE_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Plane Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

void  InspectorWindow::DrawInfinitePlaneColliderComponent(GameObject* selectedObject)
{
    InfinitePlaneCollider* Collider = static_cast<InfinitePlaneCollider*>(selectedObject->GetComponent(ComponentType::INFINITE_PLANE_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Infinite Plane Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

void  InspectorWindow::DrawMeshColliderComponent(GameObject* selectedObject)
{
    MeshCollider* Collider = static_cast<MeshCollider*>(selectedObject->GetComponent(ComponentType::MESH_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Mesh Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

void  InspectorWindow::DrawConvexColliderComponent(GameObject* selectedObject)
{
    ConvexCollider* Collider = static_cast<ConvexCollider*>(selectedObject->GetComponent(ComponentType::CONVEX_COLLIDER));

    if (Collider != nullptr)
    {
        bool open = ImGui::CollapsingHeader("Convex Collider", ImGuiTreeNodeFlags_DefaultOpen);
        DrawComponentContextMenu(Collider);
        if (open) Collider->OnEditor();
    }
}

static void DrawJointBaseSection(Joint* joint)
{
    ImGui::Text("Connections:");
    ImGui::Separator();

    if (ImGui::BeginTable("JointConnections", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Values", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Body A:");
        ImGui::TableNextColumn();
        if (joint->GetBodyA())
            ImGui::Text("%s", joint->GetBodyA()->owner->name.c_str());
        else
            ImGui::TextDisabled("None");

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Body B:");
        ImGui::TableNextColumn();
        if (joint->GetBodyB())
            ImGui::Button(joint->GetBodyB()->owner->name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 20));
        else
            ImGui::Button("None (World)", ImVec2(ImGui::GetContentRegionAvail().x, 20));

        ImGui::EndTable();
    }

    if (ImGui::Button("Clear Target"))
        joint->SetTarget(nullptr);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Body A Settings", flags))
    {
        glm::vec3 posA = joint->GetLocalPosA();
        ImGui::Text("Offset Position");
        if (ImGui::InputFloat3("##PosA", &posA.x))
            joint->SetAnchorPosition(Joint::JointBody::Self, posA);

        glm::vec3 eulerA = JointQuatToEuler(joint->GetLocalRotA());
        ImGui::Text("Offset Rotation");
        if (ImGui::InputFloat3("##RotA", &eulerA.x))
            joint->SetAnchorRotation(Joint::JointBody::Self, JointEulerToQuat(eulerA));

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Body B Settings", flags))
    {
        glm::vec3 posB = joint->GetLocalPosB();
        ImGui::Text("Offset Position");
        if (ImGui::InputFloat3("##PosB", &posB.x))
            joint->SetAnchorPosition(Joint::JointBody::Target, posB);

        glm::vec3 eulerB = JointQuatToEuler(joint->GetLocalRotB());
        ImGui::Text("Offset Rotation");
        if (ImGui::InputFloat3("##RotB", &eulerB.x))
            joint->SetAnchorRotation(Joint::JointBody::Target, JointEulerToQuat(eulerB));

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Break Settings", flags))
    {
        float breakForce = joint->GetBreakForce();
        float breakTorque = joint->GetBreakTorque();

        const char* forceFormat = (breakForce >= INFINITY_PHYSIC) ? "INFINITY" : "%.3f";
        const char* torqueFormat = (breakTorque >= INFINITY_PHYSIC) ? "INFINITY" : "%.3f";

        ImGui::Text("Break Force");
        if (ImGui::InputFloat("##BreakForce", &breakForce, 0.0f, 0.0f, forceFormat))
            joint->SetBreakForce(breakForce);

        ImGui::Text("Break Torque");
        if (ImGui::InputFloat("##BreakTorque", &breakTorque, 0.0f, 0.0f, torqueFormat))
            joint->SetBreakTorque(breakTorque);

        if (breakForce < INFINITY_PHYSIC || breakTorque < INFINITY_PHYSIC)
        {
            if (ImGui::Button("Reset##BreakReset"))
            {
                joint->SetBreakForce(INFINITY_PHYSIC);
                joint->SetBreakTorque(INFINITY_PHYSIC);
            }
        }

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawFixedJointComponent(GameObject* selectedObject)
{
    FixedJoint* joint = static_cast<FixedJoint*>(selectedObject->GetComponent(ComponentType::FIXED_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("Fixed Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (open)
        DrawJointBaseSection(joint);
}

void InspectorWindow::DrawDistanceJointComponent(GameObject* selectedObject)
{
    DistanceJoint* joint = static_cast<DistanceJoint*>(selectedObject->GetComponent(ComponentType::DISTANCE_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("Distance Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (!open) return;

    DrawJointBaseSection(joint);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Distance Settings", flags))
    {
        bool maxEnabled = joint->GetMaxDistanceEnabled();
        if (ImGui::Checkbox("Enable Max Distance", &maxEnabled))
            joint->EnableMaxDistance(maxEnabled);

        if (maxEnabled) {
            float maxDist = joint->GetMaxDistance();
            ImGui::Text("Max Distance");
            if (ImGui::InputFloat("##MaxDist", &maxDist))
                joint->SetMaxDistance(maxDist);
        }

        float minDist = joint->GetMinDistance();
        ImGui::Text("Min Distance");
        if (ImGui::InputFloat("##MinDist", &minDist))
            joint->SetMinDistance(minDist);

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Spring Settings", flags))
    {
        bool springEnabled = joint->GetSpringEnabled();
        if (ImGui::Checkbox("Enable Spring", &springEnabled))
            joint->EnableSpring(springEnabled);

        float stiffness = joint->GetStiffness();
        ImGui::Text("Stiffness");
        if (ImGui::InputFloat("##Stiffness", &stiffness))
            joint->SetStiffness(stiffness);

        float damping = joint->GetDamping();
        ImGui::Text("Damping");
        if (ImGui::InputFloat("##Damping", &damping))
            joint->SetDamping(damping);

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawHingeJointComponent(GameObject* selectedObject)
{
    HingeJoint* joint = static_cast<HingeJoint*>(selectedObject->GetComponent(ComponentType::HINGE_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("Hinge Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (!open) return;

    DrawJointBaseSection(joint);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Hinge Limits", flags))
    {
        bool limitsEnabled = joint->GetLimitsEnabled();
        if (ImGui::Checkbox("Enable Limits", &limitsEnabled))
            joint->EnableLimits(limitsEnabled);

        float minAngle = joint->GetMinAngle();
        if (ImGui::SliderFloat("Min Angle", &minAngle, -179.9f, 0.0f))
            joint->SetMinAngle(minAngle);

        float maxAngle = joint->GetMaxAngle();
        if (ImGui::SliderFloat("Max Angle", &maxAngle, 0.0f, 179.9f))
            joint->SetMaxAngle(maxAngle);

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Motor Settings", flags))
    {
        bool motorEnabled = joint->GetMotorEnabled();
        if (ImGui::Checkbox("Enable Motor", &motorEnabled))
            joint->EnableMotor(motorEnabled);

        float velocity = joint->GetDriveVelocity();
        if (ImGui::InputFloat("Velocity", &velocity))
            joint->SetDriveVelocity(velocity);

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawSphericalJointComponent(GameObject* selectedObject)
{
    SphericalJoint* joint = static_cast<SphericalJoint*>(selectedObject->GetComponent(ComponentType::SPHERICAL_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("Spherical Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (!open) return;

    DrawJointBaseSection(joint);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Cone Limit Settings", flags))
    {
        bool limitsEnabled = joint->GetLimitsEnabled();
        if (ImGui::Checkbox("Enable Cone Limit", &limitsEnabled))
            joint->EnableLimits(limitsEnabled);

        float limitAngle = joint->GetLimitAngle();
        if (ImGui::SliderFloat("Limit Angle", &limitAngle, 0.0f, 180.0f))
            joint->SetConeLimit(limitAngle);

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawPrismaticJointComponent(GameObject* selectedObject)
{
    PrismaticJoint* joint = static_cast<PrismaticJoint*>(selectedObject->GetComponent(ComponentType::PRISMATIC_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("Prismatic Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (!open) return;

    DrawJointBaseSection(joint);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Limit Settings", flags))
    {
        bool limitsEnabled = joint->GetLimitsEnabled();
        if (ImGui::Checkbox("Enable Limits", &limitsEnabled))
            joint->EnableLimits(limitsEnabled);

        float minLimit = joint->GetMinLimit();
        if (ImGui::InputFloat("Min Limit", &minLimit))
            joint->SetMinLimit(minLimit);

        float maxLimit = joint->GetMaxLimit();
        if (ImGui::InputFloat("Max Limit", &maxLimit))
            joint->SetMaxLimit(maxLimit);

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Soft Limit (Spring)", flags))
    {
        bool softEnabled = joint->GetSoftLimitEnabled();
        if (ImGui::Checkbox("Enable Soft Limit", &softEnabled))
            joint->EnableSoftLimit(softEnabled);

        float stiffness = joint->GetStiffness();
        if (ImGui::InputFloat("Stiffness", &stiffness))
            joint->SetStiffness(stiffness);

        float damping = joint->GetDamping();
        if (ImGui::InputFloat("Damping", &damping))
            joint->SetDamping(damping);

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawD6JointComponent(GameObject* selectedObject)
{
    D6Joint* joint = static_cast<D6Joint*>(selectedObject->GetComponent(ComponentType::D6_JOINT));
    if (!joint) return;

    bool open = ImGui::CollapsingHeader("D6 Joint", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(joint);
    if (!open) return;

    DrawJointBaseSection(joint);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    const char* axisNames[] = { "X", "Y", "Z", "Twist (Rot X)", "Swing Y", "Swing Z" };
    const char* motionNames[] = { "Locked", "Limited", "Free" };

    if (ImGui::TreeNodeEx("Motions", flags))
    {
        for (int i = 0; i < 6; ++i)
        {
            int current = (int)joint->GetMotion(i);
            if (ImGui::Combo(axisNames[i], &current, motionNames, 3))
                joint->SetMotion((physx::PxD6Axis::Enum)i, (physx::PxD6Motion::Enum)current);
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Limits", flags))
    {
        float linearLimit = joint->GetLinearLimit();
        if (ImGui::DragFloat("Linear Extent", &linearLimit, 0.1f, 0.0f, 100.0f))
            joint->SetLinearLimit(linearLimit);

        float twistMin = joint->GetTwistMin();
        float twistMax = joint->GetTwistMax();
        if (ImGui::DragFloat2("Twist Min/Max", &twistMin, 1.0f, -180.0f, 180.0f))
            joint->SetTwistLimit(twistMin, twistMax);

        float swingY = joint->GetSwingY();
        float swingZ = joint->GetSwingZ();
        if (ImGui::DragFloat2("Swing Y/Z", &swingY, 1.0f, 0.0f, 180.0f))
            joint->SetSwingLimit(swingY, swingZ);

        ImGui::TreePop();
    }
}

void InspectorWindow::DrawCanvasComponent(GameObject* selectedObject)
{
    ComponentCanvas* canvasComp = static_cast<ComponentCanvas*>(selectedObject->GetComponent(ComponentType::CANVAS));

    if (canvasComp == nullptr) return;

    bool open = ImGui::CollapsingHeader("Canvas", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(canvasComp); 
    if (open)
    {
        ImGui::Text("XAML File:");
        ImGui::Spacing();

        // Scan Assets/UI for .xaml files
        std::vector<std::string> xamlFiles;
        std::string uiPath = "../Assets/UI";
        if (std::filesystem::exists(uiPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(uiPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".xaml")
                    xamlFiles.push_back(entry.path().filename().string());
            }
        }

        std::string currentXAML = canvasComp->GetCurrentXAML();
        std::string currentName = currentXAML.empty() ? "None"
            : std::filesystem::path(currentXAML).filename().string();

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##XAMLSelector", currentName.c_str()))
        {
            if (ImGui::Selectable("None", currentXAML.empty()))
                canvasComp->UnloadXAML();

            for (const auto& file : xamlFiles)
            {
                bool isSelected = (currentName == file);
                if (ImGui::Selectable(file.c_str(), isSelected))
                {
                    if (canvasComp->LoadXAML(file.c_str()))
                        LOG_CONSOLE("[Inspector] XAML loaded: %s", file.c_str());
                    else
                        LOG_CONSOLE("[Inspector] Failed to load XAML: %s", file.c_str());
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            if (xamlFiles.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f),
                    "No .xaml files found in Assets/UI");
            }

            ImGui::EndCombo();
        }

        ImGui::Spacing();

        float opacity = canvasComp->GetOpacity();
        if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f))
        {
            canvasComp->SetOpacity(opacity);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        unsigned int texID = canvasComp->GetTextureID();
        if (texID != 0)
        {
            ImGui::Text("Preview:");
            ImGui::Image((ImTextureID)(uintptr_t)texID, ImVec2(256, 144), ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No XAML loaded");
        }
    }
}

void InspectorWindow::DrawAudioSourceComponent(GameObject* selectedObject) {
    AudioSource* source = static_cast<AudioSource*>(selectedObject->GetComponent(ComponentType::AUDIOSOURCE));
    if (!source) return;

    bool open = ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(source);
    if (open) {
        source->OnEditor();
    }
}

void InspectorWindow::DrawAudioListenerComponent(GameObject* selectedObject) {
    AudioListener* listener = static_cast<AudioListener*>(selectedObject->GetComponent(ComponentType::LISTENER));
    if (!listener) return;

    bool open = ImGui::CollapsingHeader("Audio Listener", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(listener);
    if (open) {
        listener->OnEditor();
    }
}

void InspectorWindow::DrawReverbZoneComponent(GameObject* selectedObject)
{
    ReverbZone* zone = static_cast<ReverbZone*>(selectedObject->GetComponent(ComponentType::REVERBZONE));
    if (!zone) return;

    bool open = ImGui::CollapsingHeader("Reverb Zone", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(zone);
    if (open) {
        zone->OnEditor();
    }
}

void InspectorWindow::DrawScriptComponent(GameObject* selectedObject)
{
    ComponentScript* scriptComp = static_cast<ComponentScript*>(
        selectedObject->GetComponent(ComponentType::SCRIPT)
        );

    if (scriptComp == nullptr) return;

    bool open = ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen);
    DrawComponentContextMenu(scriptComp);
    if (open)
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
            Component* newComp = selectedObject->CreateComponent(ComponentType::SCRIPT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );

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
            Component* newComp = selectedObject->CreateComponent(ComponentType::CAMERA);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );

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

        // Canvas Component
        bool hasCanvas = (selectedObject->GetComponent(ComponentType::CANVAS) != nullptr);
        if (hasCanvas)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }
        if (ImGui::Selectable("Canvas", false, hasCanvas ? ImGuiSelectableFlags_Disabled : 0))
        {
            selectedObject->CreateComponent(ComponentType::CANVAS);
            LOG_CONSOLE("[Inspector] Canvas component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }
        if (hasCanvas)
        {
            ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered() && !hasCanvas)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Canvas to this GameObject");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasCanvas)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has a Canvas component");
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
            Component* newComp = selectedObject->CreateComponent(ComponentType::MESH);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );

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
            Component* newComp = selectedObject->CreateComponent(ComponentType::MATERIAL);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );

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
            Component* newComp = selectedObject->CreateComponent(ComponentType::PARTICLE);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
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

        if (ImGui::Selectable("Rigidbody", false, hasRigidbody ? ImGuiSelectableFlags_Disabled : 0))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::RIGIDBODY);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
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
            Component* newComp = selectedObject->CreateComponent(ComponentType::BOX_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
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
            Component* newComp = selectedObject->CreateComponent(ComponentType::SPHERE_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Sphere Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Sphere Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Capsule Collider", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::CAPSULE_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Capsule Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Capsule Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Plane Collider", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::PLANE_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Plane Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Plane Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Infinite Plane Collider", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::INFINITE_PLANE_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Infinite Plane Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Infinite Plane Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Mesh Collider", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::MESH_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Mesh Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Mesh Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Convex Collider", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::CONVEX_COLLIDER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Convex Collider component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Convex Collider");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Fixed Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::FIXED_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Fixed Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Fixed Joint");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Distance Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::DISTANCE_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Distance Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Distance Joint");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Hinge Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::HINGE_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Hinge Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Hinge Joint");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Spherical Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::SPHERICAL_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Spherical Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Spherical Joint");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("Prismatic Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::PRISMATIC_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Prismatic Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Prismatic Joint");
            ImGui::EndTooltip();
        }

        if (ImGui::Selectable("D6 Joint", false))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::D6_JOINT);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] D6 Joint component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a D6 Joint");
            ImGui::EndTooltip();
        }
        //Audio Source Component --> 0 to multiple per object
        if (ImGui::Selectable("Audio Source"))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::AUDIOSOURCE);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );

            LOG_CONSOLE("[Inspector] AudioSource component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup(); // disappear automatically after selecting an option
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add an Audio Source");
            ImGui::EndTooltip();
        }

        //Audio Listener Component --> 0 to max 1 per object
        bool hasListener = (selectedObject->GetComponent(ComponentType::LISTENER) != nullptr);

        if (hasListener) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        if (ImGui::Selectable("Audio Listener", false, hasListener ? ImGuiSelectableFlags_Disabled : 0))
        {
            Component* newComp = selectedObject->CreateComponent(ComponentType::LISTENER);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] Audio Listener component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (hasListener) ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && !hasListener)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add an Audio Listener");
            ImGui::EndTooltip();
        }
        else if (ImGui::IsItemHovered() && hasListener)
        {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Already has an Audio Listener");
            ImGui::EndTooltip();
        }

        //Reverb Zone Component
        if (ImGui::Selectable("Reverb Zone")) {
            Component* newComp = selectedObject->CreateComponent(ComponentType::REVERBZONE);
            if (newComp)
                Application::GetInstance().editor->GetCommandHistory()->PushWithoutExecute(
                    std::make_unique<AddComponentCommand>(selectedObject, newComp)
                );
            LOG_CONSOLE("[Inspector] ReverbZone component added to: %s", selectedObject->GetName().c_str());
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Add a Reverb Zone");
            ImGui::EndTooltip();
        }

        ImGui::EndPopup();
    }
}

//Helpers
static glm::vec3 JointQuatToEuler(const glm::quat& q) {
    return glm::degrees(glm::eulerAngles(q));
}
static glm::quat JointEulerToQuat(const glm::vec3& deg) {
    return glm::quat(glm::radians(deg));
}