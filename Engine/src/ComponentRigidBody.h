#pragma once
#include "Component.h"
#include <btBulletDynamicsCommon.h>

class ComponentRigidBody : public Component
{
public:
    ComponentRigidBody(GameObject* owner);
    ~ComponentRigidBody();

    void Start();
    void Update() override; // Aquí sincronizaremos Física -> Gráficos

    // Configuración básica
    void SetMass(float mass);
    
    // Funciones internas de Bullet
    btRigidBody* GetRigidBody() const { return rigidBody; }

private:
    void AddBodyToWorld();
    void RemoveBodyFromWorld();

private:
    btRigidBody* rigidBody = nullptr;
    btCollisionShape* colShape = nullptr;
    btDefaultMotionState* motionState = nullptr;
    
    float mass = 1.0f; // 1.0 = Dinámico, 0.0 = Estático
};