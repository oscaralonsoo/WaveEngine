#pragma once
#include "Collider.h"

namespace physx { class PxConvexMesh; }

class MeshCollider : public Collider {
public:
    MeshCollider(GameObject* owner);
    ~MeshCollider();

    void Update() override;

    physx::PxGeometry* GetGeometry() override;

    bool CanBeDynamic() const override { return false; }
    ColliderType GetColliderType() override { return ColliderType::MESH_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::MESH_COLLIDER; };

    void OnEditor() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    //void Save(Config& config) override;
    //void Load(Config& config) override;

    void DebugShape();

    void OnGameObjectEvent(GameObjectEvent event, Component* component) override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:
    void CookMesh();

    physx::PxTriangleMesh* cookedMesh = nullptr;
};