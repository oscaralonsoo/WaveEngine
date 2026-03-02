#pragma once
#include "Collider.h"

namespace physx { class PxConvexMesh; }

class ConvexCollider : public Collider {
public:
    ConvexCollider(GameObject* owner);
    ~ConvexCollider();

    void Update() override;

    physx::PxGeometry* GetGeometry() override;

    ColliderType GetColliderType() override { return ColliderType::CONVEX_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::CONVEX_COLLIDER; };

    void OnEditor() override;
   
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void DebugShape();

    void OnGameObjectEvent(GameObjectEvent event, Component* component) override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:
    void CookMesh();

    physx::PxConvexMesh* cookedMesh = nullptr;
};