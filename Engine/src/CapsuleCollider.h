#pragma once
#include "Collider.h"

class GameObject;

class CapsuleCollider : public Collider {
public:
    
    CapsuleCollider(GameObject* owner);

    void Update() override;

    physx::PxGeometry* GetGeometry() override;
    ColliderType GetColliderType() override { return ColliderType::CAPSULE_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::CAPSULE_COLLIDER; };
    void OnEditor() override;
    
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void SetRadius(float radius);
    const float GetRadius() { return radius; }
    void SetHeight(float height);
    const float GetHeight() { return height; }
   
    void DebugShape();

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override;

private:
    float radius = 0.5f;
    float height = 1.0f;
};