#pragma once
#include "Component.h"
#include "Global.h"
#include "../EventListener.h"
#include "../utils/AABB.h"
#include "../resources/ResourceUser.h"

class ResourceMesh;
class ResourceTexture;
class GameObject;
class Config;

class MeshRenderer : public Component, public ResourceUser, public EventListener
{
public:
    MeshRenderer(GameObject* owner);
    virtual ~MeshRenderer() override;

    void Update() override { cachedBones = false; }
    void CleanUp() override;

    virtual ComponentType GetType() override { return ComponentType::MeshRenderer; }
    bool IsType(ComponentType type) override { return type == ComponentType::MeshRenderer; };
    bool IsIncompatible(ComponentType type) override { return type == ComponentType::MeshRenderer || type == ComponentType::SkinnedMeshRenderer;; };
    void Save(Config& componentNode) override;
    void Load(Config& componentNode) override;

    virtual AABB GetGlobalAABB();

    //TEXTURE
    virtual const bool& GetTransparent() { return transparent; }
    void SetTransparent(bool t) { transparent = t; }

    //SKINNING
    virtual void UpdateSkinningMatrices() {}
    virtual void UpdateDynamicAABB() {}
    virtual bool HasSkinning() const { return false; }
    virtual const std::vector<glm::mat4>& GetBoneMatrices() const { static std::vector<glm::mat4> empty; return empty; }

    //RESOURCES
    virtual void SetMeshResource(UID uid);
    ResourceMesh* GetMeshResource() const { return meshResource; }
    void SetTextureResource(UID uid);
    ResourceTexture* GetTextureResource() const { return textureResource; }
    void OnResourceLost(UID resourceUID) override;

    //EVENTS
    virtual void OnEvent(const Event& event) override {};

    //EDITOR
    virtual void OnEditor() override;
    void OnEditorMesh();
    void OnEditorTexture();

public:
    bool drawNormals = false;
    bool drawMesh = false;
    bool drawStencil = false;
    bool drawChecker = false;

protected:

    //MESH
    UID meshUID = 0;
    mutable ResourceMesh* meshResource = nullptr;

    //TEXTURE
    UID textureUID = 0;
    mutable ResourceTexture* textureResource = nullptr;
    bool transparent = false;

    //ANIMATION
    bool cachedBones = false;

};