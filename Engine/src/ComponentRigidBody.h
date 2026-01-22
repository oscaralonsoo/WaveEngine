#pragma once
#include "Component.h"
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

enum class ShapeType {
    BOX,
    SPHERE,
    CAPSULE,
    CYLINDER
};

class ComponentRigidBody : public Component
{
public:
    ComponentRigidBody(GameObject* owner);
    ~ComponentRigidBody();

    void Start();
    void Update() override;
    void OnEditor() override; 

    void SetMass(float mass);
    void SetCenterOffset(const glm::vec3& offset);
    void SetShapeType(ShapeType type);
    
    // Funciones internas de Bullet
    btRigidBody* GetRigidBody() const { return rigidBody; }
        void SyncToBullet();

private:
    void AddBodyToWorld();
    void RemoveBodyFromWorld();
    void UpdateRigidBodyScale();
    void CreateShape();

private:
    btRigidBody* rigidBody = nullptr;
    btCollisionShape* colShape = nullptr;
    btDefaultMotionState* motionState = nullptr;
    
    float mass = 1.0f;
    glm::vec3 centerOffset = glm::vec3(0.0f);
    glm::vec3 lastScale = glm::vec3(1.0f);

    ShapeType shapeType = ShapeType::BOX;
};