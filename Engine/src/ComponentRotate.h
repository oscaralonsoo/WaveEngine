#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class ComponentRotate : public Component {
public:
    ComponentRotate(GameObject* owner);
    ~ComponentRotate() = default;

    void Update() override;
    void OnEditor() override;

    bool IsType(ComponentType type) override { return type == ComponentType::ROTATE; };
    bool IsIncompatible(ComponentType type) override { return type == ComponentType::ROTATE; };

    void SetRotationSpeed(const glm::vec3& speed) { rotationSpeed = speed; }
    glm::vec3 GetRotationSpeed() const { return rotationSpeed; }

private:
    glm::vec3 rotationSpeed;
};
