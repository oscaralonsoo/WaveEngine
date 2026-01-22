#include "ModulePhysics.h"
#include "Application.h"

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

    // 1. Configuración de colisiones predeterminada
    collision_conf = new btDefaultCollisionConfiguration();

    // 2. Dispatcher de colisiones (gestiona los algoritmos de colisión)
    dispatcher = new btCollisionDispatcher(collision_conf);

    // 3. Broadphase (detecta pares de objetos cercanos)
    broad_phase = new btDbvtBroadphase();

    // 4. Solver (resuelve las restricciones, colisiones, uniones, etc.)
    solver = new btSequentialImpulseConstraintSolver();

    // 5. El mundo físico
    world = new btDiscreteDynamicsWorld(dispatcher, broad_phase, solver, collision_conf);

    // Configurar gravedad global (eje Y hacia abajo estándar)
    world->setGravity(btVector3(0.0f, -9.81f, 0.0f));

    return true;
}

bool ModulePhysics::PreUpdate()
{
    // Solo simulamos físicas si estamos en modo PLAYING
    // (O si implementas un modo "Step" paso a paso)
    if (Application::GetInstance().GetPlayState() == Application::PlayState::PLAYING)
    {
        // stepSimulation(timeStep, maxSubSteps, fixedTimeStep)
        // Usamos 1/60 (60 FPS) como paso fijo para consistencia física
        world->stepSimulation(1.0f / 60.0f, 10);
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

    // Borrar en orden inverso a la creación
    delete world;
    delete solver;
    delete broad_phase;
    delete dispatcher;
    delete collision_conf;

    return true;
}