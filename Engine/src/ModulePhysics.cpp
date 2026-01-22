#include "ModulePhysics.h"
#include "Application.h"
#include "PhysicsDebugDrawer.h"
#include "ComponentRigidBody.h"
#include "GameObject.h"

#include <iostream>

void SyncAllRigidBodies(GameObject* obj) {
    if (obj == nullptr) return;

    // Si tiene un RigidBody, lo sincronizamos
    ComponentRigidBody* rb = (ComponentRigidBody*)obj->GetComponent(ComponentType::RIGIDBODY);
    if (rb && rb->IsActive()) {
        // Usamos un casting manual o un método específico. 
        // SyncToBullet() debe ser público en ComponentRigidBody.h
        ((ComponentRigidBody*)rb)->SyncToBullet(); 
    }

    // Recorremos los hijos recursivamente
    for (GameObject* child : obj->GetChildren()) {
        SyncAllRigidBodies(child);
    }
}

ModulePhysics::ModulePhysics()
{
    name = "Physics";
}

ModulePhysics::~ModulePhysics()
{
}

bool ModulePhysics::Start()
{
    LOG_CONSOLE("Inicializando Physics con Bullet");

    // --- 1. Inicialización de Bullet (ESTO SE QUEDA) ---
    collision_conf = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collision_conf);
    broad_phase = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    world = new btDiscreteDynamicsWorld(dispatcher, broad_phase, solver, collision_conf);

    // Gravedad estándar
    world->setGravity(btVector3(0.0f, -9.81f, 0.0f));

    // --- 2. Inicialización del Debug Drawer (ESTO SE QUEDA) ---
    debug_draw = new PhysicsDebugDrawer();
    debug_draw->Init();
    debug_draw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    
    world->setDebugDrawer(debug_draw);

    // --- ¡BORRADO! --- 
    // Aquí es donde borramos todo lo del groundShape, sphereShape, addRigidBody manual, etc.
    // El mundo debe empezar vacío.

    return true;
}

bool ModulePhysics::PreUpdate()
{
    if (Application::GetInstance().GetPlayState() == Application::PlayState::PLAYING)
    {
        world->stepSimulation(1.0f / 60.0f, 10);
    }
    else 
    {
        // --- MODO EDITOR ---
        // Accedemos al root de la escena y sincronizamos toda la jerarquía
        GameObject* root = Application::GetInstance().scene->GetRoot();
        if (root != nullptr) {
            SyncAllRigidBodies(root);
        }
    }

    if (world)
    {
        world->debugDrawWorld();
    }

    return true;
}

bool ModulePhysics::Update()
{
    return true;
}

bool ModulePhysics::CleanUp()
{
    LOG_CONSOLE("Limpiando Physics");

    // Borrar Debug Drawer
    if (debug_draw)
    {
        delete debug_draw;
        debug_draw = nullptr;
    }

    // Borrar Bullet (En orden inverso a creación)
    // Nota: Para un cleanup limpio de verdad deberíamos borrar primero 
    // los rigidbodies que añadimos en Start(), pero al destruir el mundo
    // y cerrar la app, el SO liberará la memoria.
    delete world;
    delete solver;
    delete broad_phase;
    delete dispatcher;
    delete collision_conf;

    return true;
}

void ModulePhysics::Draw(const glm::mat4& view, const glm::mat4& projection)
{
    if (debug_draw)
    {
        debug_draw->Draw(view, projection);
    }
}