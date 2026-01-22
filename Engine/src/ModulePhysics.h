#pragma once
#include "Module.h"
#include "Globals.h"

// Bullet library
#include <btBulletDynamicsCommon.h>

class PhysicsDebugDrawer;
class GameObject;

class ModulePhysics : public Module
{
public:
    ModulePhysics();
    ~ModulePhysics();

    bool Start() override;
    bool PreUpdate() override;
    bool Update() override;
    bool CleanUp() override;

    // Getter para acceder al mundo desde otros componentes (Colliders, etc.)
    btDiscreteDynamicsWorld* GetWorld() const { return world; }
    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    // Configuración básica de Bullet
    btDefaultCollisionConfiguration* collision_conf = nullptr;
    btCollisionDispatcher* dispatcher = nullptr;
    btBroadphaseInterface* broad_phase = nullptr;
    btSequentialImpulseConstraintSolver* solver = nullptr;
    btDiscreteDynamicsWorld* world = nullptr;
    PhysicsDebugDrawer* debug_draw = nullptr;

    void ResetAllRigidBodies(GameObject* obj);

};