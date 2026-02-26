#pragma once
#include "Collider.h"

class GameObject;

class SphereCollider : public Collider {
public:
    
    SphereCollider(GameObject* owner);
    void Update() override;

    float radius = 1.0f;

    physx::PxGeometry* GetGeometry() override;
    ColliderType GetColliderType() override { return ColliderType::SPHERE_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::SPHERE_COLLIDER; };
    void OnEditor() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    //void Save(Config& config) override;
    //void Load(Config& config) override;

    const float GetRadius() { return radius; }
    void SetRadius(float radius);

    void DebugShape();

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;
};