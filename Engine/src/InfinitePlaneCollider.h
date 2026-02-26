#pragma once
#include "Collider.h"

class InfinitePlaneCollider : public Collider {
public:
    InfinitePlaneCollider(GameObject* owner);
    virtual ~InfinitePlaneCollider() = default;

    void Update() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    bool CanBeDynamic() const override { return false; }

    ColliderType GetColliderType() override { return ColliderType::INFINITE_PLANE_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::INFINITE_PLANE_COLLIDER; };

    physx::PxGeometry* GetGeometry() override;

    void OnEditor() override;
    void DebugShape() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;
};