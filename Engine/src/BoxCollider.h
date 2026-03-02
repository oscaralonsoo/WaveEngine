#pragma once
#include "Collider.h"

class GameObject;

class BoxCollider : public Collider {
public:
    
    BoxCollider(GameObject* owner);

    physx::PxGeometry* GetGeometry() override;
    ColliderType GetColliderType() override { return ColliderType::BOX_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::BOX_COLLIDER; };
    void OnEditor() override;
    void Update() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    const glm::vec3& GetSize() { return size; }
    void SetSize(glm::vec3 size);

    void DebugShape() override;

    //void Serialize(nlohmann::json& componentObj) const override;
    //void Deserialize(const nlohmann::json& componentObj) override; 

private:
    glm::vec3 size = { 1.0f, 1.0f, 1.0f };
};