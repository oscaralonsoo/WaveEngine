#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class ComponentFirework : public Component
{
public:
    ComponentFirework(GameObject* owner);
    ~ComponentFirework();

    void Update() override;

    void SetLaunchParameters(float upwardSpeed, float explosionTime, float totalLifetime);

    glm::vec4 explosionColor; // Color to use for explosion

private:
    float upwardSpeed;
    float explosionTime;
    float totalLifetime;
    float currentTime;
    bool hasExploded;
    glm::vec3 startPosition;
};
