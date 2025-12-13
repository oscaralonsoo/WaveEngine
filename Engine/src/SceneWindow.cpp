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
#include "AssetsWindow.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "Log.h"
#include "MetaFile.h"
#include "LibraryManager.h"
#include <glm/glm.hpp>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace fs = std::filesystem;

// Helper function (fuera de la clase)
static aiNode* FindNodeWithMeshIndex(aiNode* node, int meshIndex)
{
    if (!node) return nullptr;

    // Verificar si este nodo tiene la mesh que buscamos
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        if (static_cast<int>(node->mMeshes[i]) == meshIndex) {
            return node;
        }
    }

    // Buscar recursivamente en los hijos
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        aiNode* found = FindNodeWithMeshIndex(node->mChildren[i], meshIndex);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

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

        HandleAssetDropTarget();
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

void SceneWindow::HandleAssetDropTarget()
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ITEM"))
        {
            DragDropPayload* dropData = (DragDropPayload*)payload->Data;

            LOG_CONSOLE("[DROP RECEIVED] Asset: %s, Type: %d, UID: %llu",
                dropData->assetPath.c_str(),
                (int)dropData->assetType,
                dropData->assetUID);

            switch (dropData->assetType)
            {
            case DragDropAssetType::FBX_MODEL:
            {
                LOG_CONSOLE("Loading FBX model...");
                GameObject* loadedModel = Application::GetInstance().filesystem->LoadFBXAsGameObject(dropData->assetPath);

                if (loadedModel)
                {
                    GameObject* root = Application::GetInstance().scene->GetRoot();
                    root->AddChild(loadedModel);
                    Application::GetInstance().scene->RebuildOctree();
                    LOG_CONSOLE("FBX model loaded successfully");
                }
                else
                {
                    LOG_CONSOLE("ERROR: Failed to load FBX model");
                }
                break;
            }

            case DragDropAssetType::MESH:
            {
                LOG_CONSOLE("Loading mesh...");
                GameObject* meshObject = new GameObject("Mesh");

                ComponentMesh* meshComp = static_cast<ComponentMesh*>(
                    meshObject->CreateComponent(ComponentType::MESH)
                    );

                if (meshComp && meshComp->LoadMeshByUID(dropData->assetUID))
                {
                    ApplyMeshTransformFromFBX(meshObject, dropData->assetUID);

                    ComponentMaterial* matComp = static_cast<ComponentMaterial*>(
                        meshObject->CreateComponent(ComponentType::MATERIAL)
                        );

                    if (matComp)
                    {
                        unsigned long long textureUID = FindTextureForDroppedMesh(dropData->assetUID);

                        if (textureUID != 0)
                        {
                            if (matComp->LoadTextureByUID(textureUID))
                            {
                                LOG_CONSOLE("Loaded original texture (UID: %llu)", textureUID);
                            }
                            else
                            {
                                LOG_CONSOLE("Failed to load texture, using checkerboard");
                                matComp->CreateCheckerboardTexture();
                            }
                        }
                        else
                        {
                            LOG_CONSOLE("No texture found for mesh, using checkerboard");
                            matComp->CreateCheckerboardTexture();
                        }
                    }

                    GameObject* root = Application::GetInstance().scene->GetRoot();
                    root->AddChild(meshObject);
                    Application::GetInstance().scene->RebuildOctree();
                    LOG_CONSOLE("Mesh loaded successfully (UID: %llu)", dropData->assetUID);
                }
                else
                {
                    delete meshObject;
                    LOG_CONSOLE("ERROR: Failed to load mesh");
                }
                break;
            }

            case DragDropAssetType::TEXTURE:
            {
                LOG_CONSOLE("Applying texture...");

                GameObject* targetObject = GetGameObjectUnderMouse();

                if (targetObject)
                {
                    // Aplicar textura al objeto específico bajo el mouse
                    ComponentMaterial* material = static_cast<ComponentMaterial*>(
                        targetObject->GetComponent(ComponentType::MATERIAL)
                        );

                    if (!material)
                    {
                        material = static_cast<ComponentMaterial*>(
                            targetObject->CreateComponent(ComponentType::MATERIAL)
                            );
                    }

                    if (material && material->LoadTextureByUID(dropData->assetUID))
                    {
                        LOG_CONSOLE("Texture applied to: %s", targetObject->GetName().c_str());
                    }
                    else
                    {
                        LOG_CONSOLE("ERROR: Failed to apply texture to: %s", targetObject->GetName().c_str());
                    }
                }
                else
                {
                    // Fallback: aplicar a objetos seleccionados 
                    std::vector<GameObject*> selectedObjects =
                        Application::GetInstance().selectionManager->GetSelectedObjects();

                    if (selectedObjects.empty())
                    {
                        LOG_CONSOLE("No object under mouse and no selection");
                        break;
                    }

                    int successCount = 0;
                    for (GameObject* obj : selectedObjects)
                    {
                        if (!obj || !obj->IsActive())
                            continue;

                        ComponentMaterial* material = static_cast<ComponentMaterial*>(
                            obj->GetComponent(ComponentType::MATERIAL)
                            );

                        if (!material)
                        {
                            material = static_cast<ComponentMaterial*>(
                                obj->CreateComponent(ComponentType::MATERIAL)
                                );
                        }

                        if (material && material->LoadTextureByUID(dropData->assetUID))
                        {
                            successCount++;
                        }
                    }

                    if (successCount > 0)
                    {
                        LOG_CONSOLE("Texture applied to %d selected object(s)", successCount);
                    }
                    else
                    {
                        LOG_CONSOLE("ERROR: Failed to apply texture");
                    }
                }
                break;
            }

            default:
                LOG_CONSOLE("Unknown asset type dropped");
                break;
            }
        }
        ImGui::EndDragDropTarget();
    }
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

unsigned long long SceneWindow::FindTextureForDroppedMesh(unsigned long long meshUID)
{
    std::string assetsPath = LibraryManager::GetAssetsRoot();

    // 1. Buscar el FBX que contiene esta mesh
    for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        if (entry.path().extension() != ".meta") continue;

        MetaFile meta = MetaFile::Load(path);
        if (meta.type != AssetType::MODEL_FBX) continue;

        unsigned long long fbxUID = meta.uid;

        // Verificar si este FBX contiene la mesh (UIDs secuenciales)
        if (meshUID >= fbxUID && meshUID < fbxUID + 100) {
            std::string fbxPath = path.substr(0, path.length() - 5); // Quitar ".meta"

            if (!fs::exists(fbxPath)) continue;

            // 2. Calcular índice de la mesh dentro del FBX
            int meshIndex = static_cast<int>(meshUID - fbxUID);

            LOG_CONSOLE("[FindTexture] Found parent FBX: %s (mesh index: %d)", fbxPath.c_str(), meshIndex);

            // 3. Cargar FBX con Assimp para extraer info de materiales
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                fbxPath,
                aiProcess_Triangulate | aiProcess_FlipUVs
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
                LOG_CONSOLE("[FindTexture] Failed to load FBX with Assimp");
                continue;
            }

            if (meshIndex >= static_cast<int>(scene->mNumMeshes)) {
                LOG_CONSOLE("[FindTexture] Mesh index out of range");
                continue;
            }

            // 4. Obtener la mesh específica
            aiMesh* mesh = scene->mMeshes[meshIndex];

            // 5. Obtener el material de esta mesh
            if (mesh->mMaterialIndex >= scene->mNumMaterials) {
                LOG_CONSOLE("[FindTexture] Invalid material index");
                continue;
            }

            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // 6. Extraer la textura difusa
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString texPath;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);

                std::string textureName = texPath.C_Str();
                LOG_CONSOLE("[FindTexture] Texture name from FBX: %s", textureName.c_str());

                // 7. Buscar la textura en Assets/
                for (const auto& texEntry : fs::recursive_directory_iterator(assetsPath)) {
                    if (!texEntry.is_regular_file()) continue;

                    std::string filename = texEntry.path().filename().string();
                    std::string filenameNoExt = texEntry.path().stem().string();
                    std::string textureNoExt = fs::path(textureName).stem().string();

                    // Comparar nombre con y sin extensión
                    if (filename == textureName || filenameNoExt == textureNoExt) {
                        // 8. Obtener el UID de la textura desde su .meta
                        std::string texMetaPath = texEntry.path().string() + ".meta";
                        if (fs::exists(texMetaPath)) {
                            MetaFile texMeta = MetaFile::Load(texMetaPath);
                            LOG_CONSOLE("[FindTexture] Found texture UID: %llu", texMeta.uid);
                            return texMeta.uid;
                        }
                    }
                }

                LOG_CONSOLE("[FindTexture] Texture not found in Assets: %s", textureName.c_str());
            }
            else {
                LOG_CONSOLE("[FindTexture] No diffuse texture in material");
            }

            break; // FBX encontrado, salir del loop
        }
    }

    LOG_CONSOLE("[FindTexture] Could not find parent FBX or texture");
    return 0;
}

void SceneWindow::ApplyMeshTransformFromFBX(GameObject* meshObject, unsigned long long meshUID)
{
    std::string assetsPath = LibraryManager::GetAssetsRoot();

    // 1. Buscar el FBX que contiene esta mesh
    for (const auto& entry : fs::recursive_directory_iterator(assetsPath)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        if (entry.path().extension() != ".meta") continue;

        MetaFile meta = MetaFile::Load(path);
        if (meta.type != AssetType::MODEL_FBX) continue;

        unsigned long long fbxUID = meta.uid;

        // Verificar si este FBX contiene la mesh
        if (meshUID >= fbxUID && meshUID < fbxUID + 100) {
            std::string fbxPath = path.substr(0, path.length() - 5);

            if (!fs::exists(fbxPath)) continue;

            // 2. Calcular índice de la mesh
            int meshIndex = static_cast<int>(meshUID - fbxUID);

            // 3. Cargar FBX con Assimp
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(
                fbxPath,
                aiProcess_Triangulate | aiProcess_FlipUVs
            );

            if (!scene || meshIndex >= static_cast<int>(scene->mNumMeshes)) {
                continue;
            }

            // 4. Buscar el nodo que contiene esta mesh en la jerarquía
            aiNode* meshNode = FindNodeWithMeshIndex(scene->mRootNode, meshIndex);

            if (!meshNode) {
                LOG_CONSOLE("[ApplyTransform] Could not find node for mesh index %d", meshIndex);
                continue;
            }

            // 5. Acumular SOLO rotaciones y escalas, NO posiciones
            // Para esto, extraemos solo la parte de rotación/escala de cada transformación
            aiMatrix4x4 accumulatedTransform;
            aiNode* currentNode = meshNode;

            while (currentNode) {
                // Extraer la transformación del nodo
                aiMatrix4x4 nodeTransform = currentNode->mTransformation;

                // Remover la traslación (mantener solo rotación y escala)
                nodeTransform.a4 = 0.0f;
                nodeTransform.b4 = 0.0f;
                nodeTransform.c4 = 0.0f;

                // Acumular
                accumulatedTransform = nodeTransform * accumulatedTransform;
                currentNode = currentNode->mParent;
            }

            // 6. Convertir aiMatrix4x4 a glm::mat4
            glm::mat4 glmTransform;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    glmTransform[j][i] = accumulatedTransform[i][j];
                }
            }

            // 7. Descomponer la matriz en rotation y scale (sin position)
            glm::vec3 scale, skew;
            glm::vec4 perspective;
            glm::quat rotation;
            glm::vec3 translation; // Esto lo ignoramos
            glm::decompose(glmTransform, scale, rotation, translation, skew, perspective);

            // 8. Aplicar SOLO rotación y escala al Transform del GameObject
            // La posición se deja en (0, 0, 0)
            Transform* transform = static_cast<Transform*>(
                meshObject->GetComponent(ComponentType::TRANSFORM)
                );

            if (transform) {
                transform->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f)); // Forzar a origen
                transform->SetRotationQuat(rotation);
                transform->SetScale(scale);

                LOG_CONSOLE("[ApplyTransform] Applied transform - Pos: (0, 0, 0), Rot: (%.2f, %.2f, %.2f, %.2f), Scale: (%.2f, %.2f, %.2f)",
                    rotation.x, rotation.y, rotation.z, rotation.w,
                    scale.x, scale.y, scale.z);
            }

            break;
        }
    }
}

GameObject* SceneWindow::GetGameObjectUnderMouse()
{
    // Obtener posición del mouse relativa al viewport de scene
    ImVec2 mousePos = ImGui::GetMousePos();

    float relativeX = mousePos.x - sceneViewportPos.x;
    float relativeY = mousePos.y - sceneViewportPos.y;

    // Verificar que el mouse está dentro del viewport
    if (relativeX < 0 || relativeX > sceneViewportSize.x ||
        relativeY < 0 || relativeY > sceneViewportSize.y)
    {
        return nullptr;
    }

    // Obtener la cámara activa
    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (!camera)
    {
        return nullptr;
    }

    // Generar rayo desde la cámara
    glm::vec3 rayOrigin = camera->GetPosition();
    glm::vec3 rayDir = camera->ScreenToWorldRay(
        static_cast<int>(relativeX),
        static_cast<int>(relativeY),
        static_cast<int>(sceneViewportSize.x),
        static_cast<int>(sceneViewportSize.y)
    );

    // Hacer ray picking
    GameObject* root = Application::GetInstance().scene->GetRoot();
    float minDist = std::numeric_limits<float>::max();
    GameObject* hitObject = FindClosestObjectToRayOptimized(root, rayOrigin, rayDir, minDist);

    return hitObject;
}