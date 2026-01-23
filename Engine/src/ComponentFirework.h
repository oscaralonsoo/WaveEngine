#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class ComponentFirework : public Component
{
public:
    ComponentFirework(GameObject* owner);
    ~ComponentFirework();

    void Update() override;
    void OnEditor() override;

    void SetLaunchParameters(float upwardSpeed, float explosionTime, float totalLifetime);
    void Reset();

    // Getters for inspector
    float GetUpwardSpeed() const { return upwardSpeed; }
    float GetExplosionTime() const { return explosionTime; }
    float GetTotalLifetime() const { return totalLifetime; }
    float GetCurrentTime() const { return currentTime; }
    bool HasExploded() const { return hasExploded; }

    glm::vec4 explosionColor; // Color to use for explosion

private:
    void TriggerExplosion();
    void CheckAndCleanup();

    float upwardSpeed;
    float explosionTime;
    float totalLifetime;
    float currentTime;
    bool hasExploded;
    bool hasRequestedDeletion;
    glm::vec3 startPosition;
};