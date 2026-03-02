#include "SkinnedMeshRenderer.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ModuleRender.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "Transform.h"
#include "Log.h"
#include "imgui.h"

SkinnedMeshRenderer::SkinnedMeshRenderer(GameObject* owner) : MeshRenderer(owner)
{
    name = "Skinned Mesh Renderer";
    Engine::GetInstance().moduleEvents->Subscribe(Event::Type::GameObjectDestroyed, this);
    bonesLinked = false;
}

void SkinnedMeshRenderer::Update()
{
    MeshRenderer::Update();

    if (!bonesLinked && GetMeshResource() != nullptr)
    {
        LinkBones();

        if (GetMeshResource()->bones.empty() || !boneGameObjects.empty())
            bonesLinked = true;
    }
}

void SkinnedMeshRenderer::CleanUp()
{
    Engine::GetInstance().moduleRender->RemoveMesh(this);
    Engine::GetInstance().moduleEvents->Unsubscribe(Event::Type::GameObjectDestroyed, this);

    if (meshResource)
    {
        Engine::GetInstance().moduleResources->ReleaseResource(meshResource->GetUID());
        meshResource->RemoveReference(this);
        meshResource = nullptr;
        meshUID = 0;
    }

    if (textureResource)
    {
        Engine::GetInstance().moduleResources->ReleaseResource(textureResource->GetUID());
        textureResource->RemoveReference(this);
        textureResource = nullptr;
        textureUID = 0;
    }

    Engine::GetInstance().moduleRender->DeleteSSBO(ssboGlobalMatrices);
    Engine::GetInstance().moduleRender->DeleteSSBO(ssboOffsetMatrices);
}

void SkinnedMeshRenderer::SetMeshResource(UID uid)
{
    if (meshResource != nullptr)
    {
        Engine::GetInstance().moduleResources->ReleaseResource(meshResource->GetUID());
        meshResource->RemoveReference(this);
    }

    meshUID = uid;
    Resource* res = Engine::GetInstance().moduleResources->RequestResource(uid);

    if (res != nullptr && res->GetType() == Resource::Type::mesh)
    {
        meshResource = (ResourceMesh*)res;
        meshResource->AddReference(this);
        bonesLinked = false;
        boneGameObjects.clear();
    }
    else
    {
        meshResource = nullptr;
    }

    owner->PublishGameObjectEvent(GameObjectEvent::MESH_CHANGED, this);
}

void SkinnedMeshRenderer::LinkBones() {

    if (!meshResource || meshResource->bones.empty()) return;

    size_t numBones = meshResource->bones.size();
    fastBones.clear();
    fastBones.resize(numBones);
    boneGlobalMatrices.resize(numBones);
    boneGameObjects.resize(numBones);

    GameObject* root = owner;
    while (root->parent != nullptr) root = root->parent;

    std::vector<glm::mat4> offsets(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        GameObject* foundBone = root->FindChild(meshResource->bones[i].name);
        if (foundBone) {
            boneGameObjects[i] = foundBone;
            fastBones[i].globalMatrixPtr = &foundBone->transform->GetGlobalMatrix();
            fastBones[i].offsetMatrix = meshResource->bones[i].offsetMatrix;
            offsets[i] = fastBones[i].offsetMatrix;
        }
    }

    Engine::GetInstance().moduleRender->CreateSkinningSSBOs(ssboGlobalMatrices, ssboOffsetMatrices, offsets);

    bonesLinked = true;
}

 AABB SkinnedMeshRenderer::GetGlobalAABB()
{
     if(hasSkinningData) return dynamicAABB;
     return MeshRenderer::GetGlobalAABB();

    return AABB();
}


void SkinnedMeshRenderer::OnEditor()
{
    OnEditorMesh();
    ImGui::Separator();
    OnEditorTexture();
}

void SkinnedMeshRenderer::UpdateSkinningMatrices()
{
    if (!bonesLinked) return;

    meshInverseTransform = glm::inverse(owner->transform->GetGlobalMatrix());

    if (boneGlobalMatrices.size() != boneGameObjects.size())
        boneGlobalMatrices.resize(boneGameObjects.size());

    for (size_t i = 0; i < boneGameObjects.size(); ++i)
    {
        if (boneGameObjects[i])
            boneGlobalMatrices[i] = boneGameObjects[i]->transform->GetGlobalMatrix();
        else
            boneGlobalMatrices[i] = glm::mat4(1.0f);
    }

    Engine::GetInstance().moduleRender->UploadGlobalMatricesToGPU(ssboGlobalMatrices, boneGlobalMatrices);

    hasSkinningData = true;
}

void SkinnedMeshRenderer::UpdateDynamicAABB()
{
    if (!hasSkinningData || boneGameObjects.empty()) return;

    glm::vec3 minP(INFINITY);
    glm::vec3 maxP(-INFINITY);

    for (GameObject* bone : boneGameObjects)
    {
        if (bone && bone->transform)
        {
            glm::vec3 pos = bone->transform->GetGlobalPosition();
            minP = glm::min(minP, pos);
            maxP = glm::max(maxP, pos);
        }
    }

    float padding = AABB_PADDING;
    dynamicAABB.min = minP - glm::vec3(padding);
    dynamicAABB.max = maxP + glm::vec3(padding);
}

//void SkinnedMeshRenderer::OnEvent(const Event& event)
//{
//    switch (event.type)
//    {
//    case Event::Type::GameObjectDestroyed:
//    {
//        if (boneGameObjects.empty()) return;
//
//        GameObject* deletedGO = event.data.gameObject.gameObject;
//
//        for (size_t i = 0; i < boneGameObjects.size(); ++i)
//        {
//            if (boneGameObjects[i] == deletedGO)
//            {
//                boneGameObjects[i] = nullptr;
//            }
//        }
//
//        break;
//    }
//
//    default:
//        break;
//    }
//}
