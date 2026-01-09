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
#include "ComponentParticleSystem.h"
#include "ResourceTexture.h"
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
    DrawParticleSystemComponent(selectedObject);

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

    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Texture:");
        ImGui::SameLine();

        std::string currentTextureName = "None";

        if (materialComp->IsUsingCheckerboard()) {
            currentTextureName = "[Checkerboard Pattern]";
        }
        else if (materialComp->HasTexture())
        {
            UID currentUID = materialComp->GetTextureUID();
            ModuleResources* resources = Application::GetInstance().resources.get();
            const Resource* res = resources->GetResourceDirect(currentUID);
            if (res)
            {
                currentTextureName = std::string(res->GetAssetFile());
                size_t lastSlash = currentTextureName.find_last_of("/\\");
                if (lastSlash != std::string::npos)
                    currentTextureName = currentTextureName.substr(lastSlash + 1);
            }
            else
            {
                currentTextureName = "UID " + std::to_string(currentUID);
            }
        }

        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##TextureSelector", currentTextureName.c_str()))
        {
            bool isCheckerboardSelected = materialComp->IsUsingCheckerboard();
            if (ImGui::Selectable("[Checkerboard Pattern]", isCheckerboardSelected))
            {
                materialComp->CreateCheckerboardTexture();
                LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
                LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
            }

            if (isCheckerboardSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::Separator();

            ModuleResources* resources = Application::GetInstance().resources.get();
            const std::map<UID, Resource*>& allResources = resources->GetAllResources();

            for (const auto& pair : allResources)
            {
                const Resource* res = pair.second;
                if (res->GetType() == Resource::TEXTURE)
                {
                    std::string textureName = res->GetAssetFile();

                    size_t lastSlash = textureName.find_last_of("/\\");
                    if (lastSlash != std::string::npos) textureName = textureName.substr(lastSlash + 1);

                    UID textureUID = res->GetUID();
                    bool isSelected = (!materialComp->IsUsingCheckerboard() &&
                        materialComp->HasTexture() &&
                        materialComp->GetTextureUID() == textureUID);

                    std::string displayName = textureName;
                    if (res->IsLoadedToMemory())
                    {
                        displayName += " [Loaded]";
                    }

                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    unsigned int gpuID = texRes->GetGPU_ID();

                    if (ImGui::Selectable(displayName.c_str(), isSelected))
                    {
                        if (materialComp->LoadTextureByUID(textureUID))
                        {
                            LOG_DEBUG("Assigned texture '%s' (UID %llu) to GameObject '%s'",
                                textureName.c_str(), textureUID, selectedObject->GetName().c_str());
                            LOG_CONSOLE("Texture '%s' assigned to '%s'",
                                textureName.c_str(), selectedObject->GetName().c_str());
                        }
                        else
                        {
                            LOG_CONSOLE("Failed to load texture '%s' (UID %llu)", textureName.c_str(), textureUID);
                        }
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();

                        if (gpuID != 0)
                        {
                            float tooltipPreviewSize = 128.0f;
                            float width = (float)texRes->GetWidth();
                            float height = (float)texRes->GetHeight();
                            float scale = tooltipPreviewSize / std::max(width, height);
                            ImVec2 tooltipSize(width * scale, height * scale);

                            ImGui::Image((ImTextureID)(intptr_t)gpuID, tooltipSize);
                            ImGui::Separator();
                        }

                        ImGui::Text("%s", textureName.c_str());
                        ImGui::Text("UID: %llu", textureUID);
                        ImGui::Text("Size: %d x %d", texRes->GetWidth(), texRes->GetHeight());
                        ImGui::Text("Path: %s", res->GetAssetFile());
                        if (res->IsLoadedToMemory())
                        {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Not loaded");
                        }
                        ImGui::EndTooltip();
                    }
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (materialComp->HasTexture())
        {
            unsigned int gpuID = 0;
            int texWidth = 0;
            int texHeight = 0;

            if (materialComp->IsUsingCheckerboard())
            {
                Renderer* renderer = Application::GetInstance().renderer.get();
                if (renderer)
                {
                    Texture* defaultTex = renderer->GetDefaultTexture();
                    if (defaultTex)
                    {
                        gpuID = defaultTex->GetID();
                        texWidth = defaultTex->GetWidth();
                        texHeight = defaultTex->GetHeight();
                    }
                }
            }
            else
            {
                UID currentUID = materialComp->GetTextureUID();
                ModuleResources* resources = Application::GetInstance().resources.get();
                const Resource* res = resources->GetResourceDirect(currentUID);

                if (res && res->GetType() == Resource::TEXTURE)
                {
                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    gpuID = texRes->GetGPU_ID();
                    texWidth = texRes->GetWidth();
                    texHeight = texRes->GetHeight();
                }
            }

            if (gpuID != 0)
            {
                ImGui::Text("Texture Preview:");

                float previewMaxSize = 256.0f;
                float width = (float)texWidth;
                float height = (float)texHeight;

                float scale = previewMaxSize / std::max(width, height);
                ImVec2 previewSize(width * scale, height * scale);

                float windowWidth = ImGui::GetContentRegionAvail().x;
                float offsetX = (windowWidth - previewSize.x) * 0.5f;
                if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

                ImGui::Image((ImTextureID)(intptr_t)gpuID, previewSize);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            ImGui::Text("Size: %d x %d pixels", materialComp->GetTextureWidth(), materialComp->GetTextureHeight());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Actions:");
        ImGui::Spacing();

        if (ImGui::Button("Apply Checkerboard", ImVec2(-1, 0)))
        {
            materialComp->CreateCheckerboardTexture();
            LOG_DEBUG("Applied checkerboard texture to: %s", selectedObject->GetName().c_str());
            LOG_CONSOLE("Checkerboard texture applied to %s", selectedObject->GetName().c_str());
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Applies the default black and white checkerboard pattern");
        }

        ImGui::Spacing();

        if (materialComp->HasOriginalTexture())
        {
            if (ImGui::Button("Restore Original", ImVec2(-1, 0)))
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

void InspectorWindow::DrawParticleSystemComponent(GameObject* selectedObject)
{
    ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(selectedObject->GetComponent(ComponentType::PARTICLE_SYSTEM));

    if (!ps) return;

    ImGui::PushID(ps);

    if (ImGui::CollapsingHeader("Particle System", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool active = ps->IsActive();
        if (ImGui::Checkbox("Active##PS", &active)) {
            ps->SetActive(active);
        }

        ImGui::Separator();

        // PLAYBACK CONTROLS
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        if (ImGui::Button("Play##PSPlay")) ps->Play();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Stop##PSStop")) ps->Stop();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Pause##PSPause")) ps->Pause();

        ImGui::SameLine();
        if (ImGui::Button("Restart##PSRestart")) ps->Restart();

        ImGui::SameLine();
        if (ImGui::Button("Clear##PSClear")) ps->Clear();

        // Estado del sistema con más información
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("System Info:");

        const char* statusText = ps->IsPlaying() ? (ps->IsPaused() ? "Paused" : "Playing") : "Stopped";
        ImVec4 statusColor = ps->IsPlaying() ?
            (ps->IsPaused() ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f)) :
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::Text("Status:");
        ImGui::SameLine();
        ImGui::TextColored(statusColor, "%s", statusText);

        // Indicador visual si hay partículas activas
        if (ps->GetActiveParticleCount() > 0) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Active");
        }
        else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Inactive");
        }

        ImGui::Separator();

        ParticleSystemConfig& main = ps->GetMainConfig();

        // MAIN MODULE
        if (ImGui::CollapsingHeader("Main Module", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat("Duration##MainDuration", &main.duration, 0.1f, 0.1f, 1000.0f);
            ImGui::Checkbox("Looping##MainLooping", &main.looping);
            ImGui::Checkbox("Prewarm##MainPrewarm", &main.prewarm);
            if (main.prewarm) ImGui::TextDisabled("(Only works with Looping)");

            ImGui::DragFloat("Start Delay##MainDelay", &main.startDelay, 0.1f, 0.0f, 100.0f);

            ImGui::Spacing();
            ImGui::Text("Start Lifetime");
            ImGui::DragFloat("##MainStartLifetime", &main.startLifetime, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Variance##MainLifetimeVar", &main.startLifetimeVariance, 0.05f, 0.0f, 50.0f);

            ImGui::Spacing();
            ImGui::Text("Start Speed");
            ImGui::DragFloat("##MainStartSpeed", &main.startSpeed, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Variance##MainSpeedVar", &main.startSpeedVariance, 0.05f, 0.0f, 50.0f);

            ImGui::Spacing();
            ImGui::Text("Start Size");
            ImGui::DragFloat("##MainStartSize", &main.startSize, 0.01f, 0.01f, 100.0f);
            ImGui::DragFloat("Variance##MainSizeVar", &main.startSizeVariance, 0.01f, 0.0f, 50.0f);

            ImGui::Spacing();
            ImGui::Text("Start Rotation");
            ImGui::SliderFloat("##MainStartRotation", &main.startRotation, 0.0f, 360.0f);
            ImGui::SliderFloat("Variance##MainRotationVar", &main.startRotationVariance, 0.0f, 360.0f);

            ImGui::Spacing();
            ImGui::ColorEdit4("Start Color##MainColor", &main.startColor[0]);

            ImGui::Spacing();
            ImGui::DragFloat("Gravity Modifier##MainGravity", &main.gravityModifier, 0.01f, -10.0f, 10.0f);

            ImGui::Spacing();
            const char* simSpaceItems[] = { "Local", "World" };
            int simSpace = (int)main.simulationSpace;
            if (ImGui::Combo("Simulation Space##MainSimSpace", &simSpace, simSpaceItems, 2)) {
                main.simulationSpace = (ParticleSystemConfig::SimulationSpace)simSpace;
            }

            ImGui::DragInt("Max Particles##MainMaxParticles", &main.maxParticles, 10, 1, 100000);
        }

        // EMISSION MODULE
        EmissionModule& emission = ps->GetEmissionModule();

        if (ImGui::CollapsingHeader("Emission##EmissionHeader"))
        {
            ImGui::Checkbox("Enabled##EmissionEnabled", &emission.enabled);

            if (emission.enabled)
            {
                ImGui::DragFloat("Rate over Time##EmissionRateTime", &emission.rateOverTime, 0.5f, 0.0f, 10000.0f);
                ImGui::DragFloat("Rate over Distance##EmissionRateDist", &emission.rateOverDistance, 0.1f, 0.0f, 1000.0f);

                ImGui::Spacing();
                ImGui::Text("Bursts:");

                for (size_t i = 0; i < emission.bursts.size(); ++i)
                {
                    ImGui::PushID((int)i);
                    auto& burst = emission.bursts[i];

                    ImGui::Text("Burst %d:", (int)i);
                    ImGui::DragFloat("Time##BurstTime", &burst.time, 0.1f, 0.0f, main.duration);
                    ImGui::DragInt("Count##BurstCount", &burst.count, 1, 1, 10000);
                    ImGui::DragInt("Cycles##BurstCycles", &burst.cycles, 1, 1, 100);
                    ImGui::DragFloat("Repeat Interval##BurstInterval", &burst.repeatInterval, 0.1f, 0.1f, 100.0f);

                    if (ImGui::Button("Remove Burst##BurstRemove")) {
                        emission.bursts.erase(emission.bursts.begin() + i);
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                    ImGui::Separator();
                }

                if (ImGui::Button("Add Burst##BurstAdd"))
                {
                    EmissionModule::Burst newBurst;
                    emission.bursts.push_back(newBurst);
                }
            }
        }

        // SHAPE MODULE
        ShapeModule& shape = ps->GetShapeModule();

        if (ImGui::CollapsingHeader("Shape##ShapeHeader"))
        {
            ImGui::Checkbox("Enabled##ShapeEnabled", &shape.enabled);

            if (shape.enabled)
            {
                const char* shapeItems[] = { "Cone", "Sphere", "Hemisphere", "Circle", "Box", "Edge" };
                int shapeType = (int)shape.shape;
                if (ImGui::Combo("Shape##ShapeType", &shapeType, shapeItems, 6)) {
                    shape.shape = (EmitterShape)shapeType;
                }

                ImGui::Spacing();

                switch (shape.shape)
                {
                case EmitterShape::CONE:
                {
                    ImGui::SliderFloat("Angle##ConeAngle", &shape.coneAngle, 0.0f, 90.0f);
                    ImGui::DragFloat("Radius##ConeRadius", &shape.coneRadius, 0.1f, 0.0f, 100.0f);
                    ImGui::DragFloat("Length##ConeLength", &shape.coneLength, 0.1f, 0.0f, 100.0f);
                    ImGui::Checkbox("Emit from Base##ConeBase", &shape.emitFromBase);
                    break;
                }
                case EmitterShape::SPHERE:
                case EmitterShape::HEMISPHERE:
                {
                    ImGui::DragFloat("Radius##SphereRadius", &shape.sphereRadius, 0.1f, 0.1f, 100.0f);
                    ImGui::SliderFloat("Radius Thickness##SphereThickness", &shape.sphereRadiusThickness, 0.0f, 1.0f);
                    ImGui::TextDisabled("0 = Surface, 1 = Volume");
                    break;
                }
                case EmitterShape::CIRCLE:
                {
                    ImGui::DragFloat("Radius##CircleRadius", &shape.circleRadius, 0.1f, 0.1f, 100.0f);
                    ImGui::SliderFloat("Arc##CircleArc", &shape.circleArc, 0.0f, 360.0f);

                    const char* modeItems[] = { "Random", "Loop", "Ping Pong" };
                    int mode = (int)shape.circleMode;
                    if (ImGui::Combo("Mode##CircleMode", &mode, modeItems, 3)) {
                        shape.circleMode = (EmissionMode)mode;
                    }

                    ImGui::DragFloat("Speed##CircleSpeed", &shape.circleSpeed, 0.1f, 0.0f, 10.0f);
                    ImGui::DragFloat("Spread##CircleSpread", &shape.circleSpread, 0.1f, 0.0f, 10.0f);
                    break;
                }
                case EmitterShape::BOX:
                {
                    ImGui::DragFloat3("Size##BoxSize", &shape.boxSize[0], 0.1f, 0.1f, 100.0f);
                    ImGui::Checkbox("Emit from Volume##BoxVolume", &shape.emitFromVolume);
                    break;
                }
                case EmitterShape::EDGE:
                {
                    ImGui::DragFloat("Radius##EdgeRadius", &shape.edgeRadius, 0.1f, 0.1f, 100.0f);
                    break;
                }
                }

                ImGui::Spacing();
                ImGui::SliderFloat("Random Direction##ShapeRandDir", &shape.randomDirectionAmount, 0.0f, 1.0f);
                ImGui::SliderFloat("Spherical Direction##ShapeSphDir", &shape.sphericalDirectionAmount, 0.0f, 1.0f);
            }
        }

        // VELOCITY OVER LIFETIME
        VelocityOverLifetimeModule& vel = ps->GetVelocityOverLifetime();

        if (ImGui::CollapsingHeader("Velocity over Lifetime##VelHeader"))
        {
            ImGui::Checkbox("Enabled##VelEnabled", &vel.enabled);

            if (vel.enabled)
            {
                ImGui::DragFloat3("Velocity##VelVelocity", &vel.velocity[0], 0.1f, -100.0f, 100.0f);
                ImGui::Checkbox("Local Space##VelLocal", &vel.isLocal);
            }
        }

        // LIMIT VELOCITY OVER LIFETIME
        LimitVelocityOverLifetimeModule& limitVel = ps->GetLimitVelocityOverLifetime();

        if (ImGui::CollapsingHeader("Limit Velocity over Lifetime##LimitVelHeader"))
        {
            ImGui::Checkbox("Enabled##LimitVelEnabled", &limitVel.enabled);

            if (limitVel.enabled)
            {
                ImGui::DragFloat("Speed##LimitVelSpeed", &limitVel.speed, 0.1f, 0.0f, 100.0f);
                ImGui::SliderFloat("Dampen##LimitVelDampen", &limitVel.dampen, 0.0f, 1.0f);
            }
        }

        // FORCE OVER LIFETIME
        ForceOverLifetimeModule& force = ps->GetForceOverLifetime();

        if (ImGui::CollapsingHeader("Force over Lifetime##ForceHeader"))
        {
            ImGui::Checkbox("Enabled##ForceEnabled", &force.enabled);

            if (force.enabled)
            {
                ImGui::DragFloat3("Force##ForceValue", &force.force[0], 0.1f, -100.0f, 100.0f);
                ImGui::Checkbox("Local Space##ForceLocal", &force.isLocal);
            }
        }

        // COLOR OVER LIFETIME
        ColorOverLifetimeModule& col = ps->GetColorOverLifetime();

        if (ImGui::CollapsingHeader("Color over Lifetime##ColorHeader"))
        {
            ImGui::Checkbox("Enabled##ColorEnabled", &col.enabled);

            if (col.enabled)
            {
                ImGui::ColorEdit4("Start Color##ColorStart", &col.startColor[0]);
                ImGui::ColorEdit4("End Color##ColorEnd", &col.endColor[0]);

                ImGui::Spacing();
                ImGui::Text("Gradient Keys:");

                for (size_t i = 0; i < col.gradient.size(); ++i)
                {
                    ImGui::PushID((int)(1000 + i)); // Unique ID range for gradient keys
                    auto& key = col.gradient[i];

                    ImGui::SliderFloat("Time##GradTime", &key.time, 0.0f, 1.0f);
                    ImGui::ColorEdit4("Color##GradColor", &key.color[0]);

                    if (ImGui::Button("Remove Key##GradRemove")) {
                        col.gradient.erase(col.gradient.begin() + i);
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("Add Gradient Key##GradAdd"))
                {
                    ColorOverLifetimeModule::ColorKey newKey;
                    newKey.time = 0.5f;
                    newKey.color = glm::vec4(1.0f);
                    col.gradient.push_back(newKey);
                }
            }
        }

        // COLOR BY SPEED
        ColorBySpeedModule& colSpeed = ps->GetColorBySpeed();

        if (ImGui::CollapsingHeader("Color by Speed##ColorSpeedHeader"))
        {
            ImGui::Checkbox("Enabled##ColorSpeedEnabled", &colSpeed.enabled);

            if (colSpeed.enabled)
            {
                ImGui::ColorEdit4("Min Color##ColorSpeedMin", &colSpeed.minColor[0]);
                ImGui::ColorEdit4("Max Color##ColorSpeedMax", &colSpeed.maxColor[0]);
                ImGui::DragFloat("Min Speed##ColorSpeedMinVal", &colSpeed.minSpeed, 0.1f, 0.0f, 1000.0f);
                ImGui::DragFloat("Max Speed##ColorSpeedMaxVal", &colSpeed.maxSpeed, 0.1f, 0.0f, 1000.0f);
            }
        }

        // SIZE OVER LIFETIME
        SizeOverLifetimeModule& size = ps->GetSizeOverLifetime();

        if (ImGui::CollapsingHeader("Size over Lifetime##SizeHeader"))
        {
            ImGui::Checkbox("Enabled##SizeEnabled", &size.enabled);

            if (size.enabled)
            {
                ImGui::DragFloat("Start Size##SizeStart", &size.startSize, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("End Size##SizeEnd", &size.endSize, 0.01f, 0.0f, 10.0f);

                ImGui::Spacing();
                ImGui::Text("Size Curve:");

                for (size_t i = 0; i < size.sizeCurve.size(); ++i)
                {
                    ImGui::PushID((int)(2000 + i)); // Unique ID range for size keys
                    auto& key = size.sizeCurve[i];

                    ImGui::SliderFloat("Time##SizeCurveTime", &key.time, 0.0f, 1.0f);
                    ImGui::DragFloat("Size##SizeCurveSize", &key.size, 0.01f, 0.0f, 10.0f);

                    if (ImGui::Button("Remove Key##SizeCurveRemove")) {
                        size.sizeCurve.erase(size.sizeCurve.begin() + i);
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID();
                }

                if (ImGui::Button("Add Size Key##SizeCurveAdd"))
                {
                    SizeOverLifetimeModule::SizeKey newKey;
                    newKey.time = 0.5f;
                    newKey.size = 1.0f;
                    size.sizeCurve.push_back(newKey);
                }
            }
        }

        // SIZE BY SPEED
        SizeBySpeedModule& sizeSpeed = ps->GetSizeBySpeed();

        if (ImGui::CollapsingHeader("Size by Speed##SizeSpeedHeader"))
        {
            ImGui::Checkbox("Enabled##SizeSpeedEnabled", &sizeSpeed.enabled);

            if (sizeSpeed.enabled)
            {
                ImGui::DragFloat("Min Size##SizeSpeedMin", &sizeSpeed.minSize, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Max Size##SizeSpeedMax", &sizeSpeed.maxSize, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Min Speed##SizeSpeedMinVal", &sizeSpeed.minSpeed, 0.1f, 0.0f, 1000.0f);
                ImGui::DragFloat("Max Speed##SizeSpeedMaxVal", &sizeSpeed.maxSpeed, 0.1f, 0.0f, 1000.0f);
            }
        }

        // ROTATION OVER LIFETIME
        RotationOverLifetimeModule& rot = ps->GetRotationOverLifetime();

        if (ImGui::CollapsingHeader("Rotation over Lifetime##RotHeader"))
        {
            ImGui::Checkbox("Enabled##RotEnabled", &rot.enabled);

            if (rot.enabled)
            {
                ImGui::DragFloat("Angular Velocity##RotAngVel", &rot.angularVelocity, 1.0f, -1000.0f, 1000.0f);
                ImGui::DragFloat("Variance##RotAngVelVar", &rot.angularVelocityVariance, 1.0f, 0.0f, 1000.0f);
            }
        }

        // ROTATION BY SPEED
        RotationBySpeedModule& rotSpeed = ps->GetRotationBySpeed();

        if (ImGui::CollapsingHeader("Rotation by Speed##RotSpeedHeader"))
        {
            ImGui::Checkbox("Enabled##RotSpeedEnabled", &rotSpeed.enabled);

            if (rotSpeed.enabled)
            {
                ImGui::DragFloat("Min Angular Velocity##RotSpeedMin", &rotSpeed.minAngularVelocity, 1.0f, -1000.0f, 1000.0f);
                ImGui::DragFloat("Max Angular Velocity##RotSpeedMax", &rotSpeed.maxAngularVelocity, 1.0f, -1000.0f, 1000.0f);
                ImGui::DragFloat("Min Speed##RotSpeedMinVal", &rotSpeed.minSpeed, 0.1f, 0.0f, 1000.0f);
                ImGui::DragFloat("Max Speed##RotSpeedMaxVal", &rotSpeed.maxSpeed, 0.1f, 0.0f, 1000.0f);
            }
        }

        // NOISE
        NoiseModule& noise = ps->GetNoise();

        if (ImGui::CollapsingHeader("Noise##NoiseHeader"))
        {
            ImGui::Checkbox("Enabled##NoiseEnabled", &noise.enabled);

            if (noise.enabled)
            {
                ImGui::DragFloat("Strength##NoiseStrength", &noise.strength, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Frequency##NoiseFreq", &noise.frequency, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Scroll Speed##NoiseScroll", &noise.scrollSpeed, 0.1f, 0.0f, 10.0f);
                ImGui::Checkbox("Damping##NoiseDamp", &noise.damping);
            }
        }

        // RENDERING
        if (ImGui::CollapsingHeader("Renderer##RendererHeader"))
        {
            const char* renderModeNames[] = { "Billboard", "Stretched Billboard", "Horizontal Billboard", "Vertical Billboard", "Mesh" };
            int currentRenderMode = (int)main.renderMode;
            if (ImGui::Combo("Render Mode##RenderMode", &currentRenderMode, renderModeNames, 5)) {
                main.renderMode = (ParticleSystemConfig::RenderMode)currentRenderMode;
            }

            const char* blendModeNames[] = { "Alpha Blend", "Additive", "Subtractive", "Multiply" };
            int currentBlendMode = (int)main.blendMode;
            if (ImGui::Combo("Blend Mode##BlendMode", &currentBlendMode, blendModeNames, 4)) {
                main.blendMode = (ParticleSystemConfig::BlendMode)currentBlendMode;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // NUEVA SECCIÓN: Selector de textura como en Material
            ImGui::Text("Texture:");
            ImGui::SameLine();

            std::string currentTextureName = "None";

            if (main.textureUID != 0)
            {
                ModuleResources* resources = Application::GetInstance().resources.get();
                const Resource* res = resources->GetResourceDirect(main.textureUID);
                if (res)
                {
                    currentTextureName = std::string(res->GetAssetFile());
                    size_t lastSlash = currentTextureName.find_last_of("/\\");
                    if (lastSlash != std::string::npos)
                        currentTextureName = currentTextureName.substr(lastSlash + 1);
                }
                else
                {
                    currentTextureName = "UID " + std::to_string(main.textureUID);
                }
            }

            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##ParticleTextureSelector", currentTextureName.c_str()))
            {
                // Opción "None"
                bool isNoneSelected = (main.textureUID == 0);
                if (ImGui::Selectable("[None]", isNoneSelected))
                {
                    main.textureUID = 0;
                    main.texturePath = "";
                    LOG_CONSOLE("Particle texture cleared");
                }

                if (isNoneSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::Separator();

                // Lista de texturas disponibles
                ModuleResources* resources = Application::GetInstance().resources.get();
                const std::map<UID, Resource*>& allResources = resources->GetAllResources();

                for (const auto& pair : allResources)
                {
                    const Resource* res = pair.second;
                    if (res->GetType() == Resource::TEXTURE)
                    {
                        std::string textureName = res->GetAssetFile();

                        size_t lastSlash = textureName.find_last_of("/\\");
                        if (lastSlash != std::string::npos)
                            textureName = textureName.substr(lastSlash + 1);

                        UID textureUID = res->GetUID();
                        bool isSelected = (main.textureUID == textureUID);

                        std::string displayName = textureName;
                        if (res->IsLoadedToMemory())
                        {
                            displayName += " [Loaded]";
                        }

                        const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                        unsigned int gpuID = texRes->GetGPU_ID();

                        if (ImGui::Selectable(displayName.c_str(), isSelected))
                        {
                            if (ps->LoadTexture(res->GetAssetFile()))
                            {
                                LOG_DEBUG("Assigned particle texture '%s' (UID %llu)",
                                    textureName.c_str(), textureUID);
                                LOG_CONSOLE("Particle texture '%s' assigned", textureName.c_str());
                            }
                            else
                            {
                                LOG_CONSOLE("Failed to load particle texture '%s'", textureName.c_str());
                            }
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }

                        // Tooltip con preview
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();

                            if (gpuID != 0)
                            {
                                float tooltipPreviewSize = 128.0f;
                                float width = (float)texRes->GetWidth();
                                float height = (float)texRes->GetHeight();
                                float scale = tooltipPreviewSize / std::max(width, height);
                                ImVec2 tooltipSize(width * scale, height * scale);

                                ImGui::Image((ImTextureID)(intptr_t)gpuID, tooltipSize);
                                ImGui::Separator();
                            }

                            ImGui::Text("%s", textureName.c_str());
                            ImGui::Text("UID: %llu", textureUID);
                            ImGui::Text("Size: %d x %d", texRes->GetWidth(), texRes->GetHeight());
                            ImGui::Text("Path: %s", res->GetAssetFile());
                            if (res->IsLoadedToMemory())
                            {
                                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Loaded in memory");
                            }
                            else
                            {
                                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Not loaded");
                            }
                            ImGui::EndTooltip();
                        }
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Preview de la textura actual
            if (main.textureUID != 0)
            {
                ModuleResources* resources = Application::GetInstance().resources.get();
                const Resource* res = resources->GetResourceDirect(main.textureUID);

                if (res && res->GetType() == Resource::TEXTURE)
                {
                    const ResourceTexture* texRes = static_cast<const ResourceTexture*>(res);
                    unsigned int gpuID = texRes->GetGPU_ID();
                    int texWidth = texRes->GetWidth();
                    int texHeight = texRes->GetHeight();

                    if (gpuID != 0)
                    {
                        ImGui::Text("Texture Preview:");

                        float previewMaxSize = 256.0f;
                        float width = (float)texWidth;
                        float height = (float)texHeight;

                        float scale = previewMaxSize / std::max(width, height);
                        ImVec2 previewSize(width * scale, height * scale);

                        float windowWidth = ImGui::GetContentRegionAvail().x;
                        float offsetX = (windowWidth - previewSize.x) * 0.5f;
                        if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

                        ImGui::Image((ImTextureID)(intptr_t)gpuID, previewSize);

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                    }

                    ImGui::Text("Size: %d x %d pixels", texWidth, texHeight);
                }
            }
        }

        ImGui::Separator();

        // PRESETS
        if (ImGui::CollapsingHeader("Presets##PresetsHeader"))
        {
            if (ImGui::Button("Smoke##PresetSmoke")) { ps->LoadSmokePreset(); ps->Play(); }
            ImGui::SameLine();
            if (ImGui::Button("Fire##PresetFire")) { ps->LoadFirePreset(); ps->Play(); }
            ImGui::SameLine();
            if (ImGui::Button("Explosion##PresetExplosion")) { ps->LoadExplosionPreset(); ps->Play(); }

            if (ImGui::Button("Sparkles##PresetSparkles")) { ps->LoadSparklesPreset(); ps->Play(); }
            ImGui::SameLine();
            if (ImGui::Button("Rain##PresetRain")) { ps->LoadRainPreset(); ps->Play(); }
            ImGui::SameLine();
            if (ImGui::Button("Snow##PresetSnow")) { ps->LoadSnowPreset(); ps->Play(); }
        }

        ImGui::Separator();

        // SAVE/LOAD
        if (ImGui::Button("Save Config...##SaveConfig"))
        {
            std::string filename = "../Assets/ParticleConfigs/config.json";
            if (ps->SaveConfig(filename))
            {
                LOG_CONSOLE("Config saved");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Config...##LoadConfig"))
        {
            std::string filename = "../Assets/ParticleConfigs/config.json";
            if (ps->LoadConfig(filename))
            {
                LOG_CONSOLE("Config loaded");
            }
        }
    }

    ImGui::PopID();
}