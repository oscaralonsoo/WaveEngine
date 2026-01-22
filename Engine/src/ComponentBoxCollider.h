#pragma once
#include "ComponentCollider.h"
#include <glm/glm.hpp>

class ComponentBoxCollider : public ComponentCollider {
public:
    ComponentBoxCollider(GameObject* owner);
    virtual ~ComponentBoxCollider() = default;

    void SetSize(const glm::vec3& size); // Declaración
    glm::vec3 GetSize() const { return size; }

    void OnEditor() override;

private:
    glm::vec3 size = glm::vec3(1.0f);
};