#pragma once
#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

class ComponentRigidBody;

class ComponentP2PConstraint : public Component
{
public:
    ComponentP2PConstraint(GameObject* owner);
    ~ComponentP2PConstraint();

    void Start();
    void OnEditor() override;

    void CreateConstraint();
    void RemoveConstraint();

    void CalculateAutoPivots();

    void SetTarget(GameObject* target) { targetObject = target; }
    void SetPivots(const glm::vec3& localA, const glm::vec3& localB) {
        pivotA = localA;
        pivotB = localB;
    }

private:
    btPoint2PointConstraint* constraint = nullptr;

    GameObject* targetObject = nullptr; // El otro objeto al que nos unimos
    
    glm::vec3 pivotA = glm::vec3(0.0f); // Punto de anclaje en este objeto (Local)
    glm::vec3 pivotB = glm::vec3(0.0f); // Punto de anclaje en el objeto B (Local)

    // Parámetros de Bullet
    float tau = 0.3f;      // Factor de corrección
    float damping = 1.0f;  // Amortiguación
    float impulseClamp = 0.0f; 
};