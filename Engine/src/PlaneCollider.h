#pragma once
#include "Collider.h"

class PlaneCollider : public Collider {
public:
    PlaneCollider(GameObject* owner);
    virtual ~PlaneCollider() = default;

    void Update() override;

    ColliderType GetColliderType() override { return ColliderType::PLANE_COLLIDER; }
    bool IsType(ComponentType type) override { return type == ComponentType::COLLIDER || type == ComponentType::PLANE_COLLIDER; };

    physx::PxGeometry* GetGeometry() override;
    const glm::vec2& GetSize() { return size; }
    void SetSize(glm::vec2 size);

    //void Save(Config& config) override;
    //void Load(Config& config) override;

    void OnEditor() override;
    void DebugShape() override;

private:
    glm::vec2 size = { 1.0f, 1.0f };
};