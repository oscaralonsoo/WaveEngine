#include "ModuleCamera.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "Input.h"
#include "Window.h"
#include "ComponentRigidBody.h"
#include "ComponentSphereCollider.h"

ModuleCamera::ModuleCamera() : 
    Module(),
    editorCameraGO(nullptr),
    editorCamera(nullptr),
    sceneCamera(nullptr),
    useEditorCamera(true)
{
}

ModuleCamera::~ModuleCamera()
{
}

bool ModuleCamera::Start()
{
    LOG_DEBUG("Initializing Camera Module");

    editorCameraGO = new GameObject("Editor Camera");

    // Set initial position for editorcamera
    Transform* transform = static_cast<Transform*>(editorCameraGO->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        transform->SetPosition(glm::vec3(0.0f, 1.5f, 10.0f));
    }

    editorCamera = static_cast<ComponentCamera*>(editorCameraGO->CreateComponent(ComponentType::CAMERA));

    ComponentRigidBody* rb = (ComponentRigidBody*)editorCameraGO->CreateComponent(ComponentType::RIGIDBODY);
    if (rb)
    {
        rb->SetMass(1.0f); // Masa necesaria para colisionar
        
        // Configuración especial para cámara "voladora"
        if (rb->GetRigidBody())
        {
            rb->GetRigidBody()->setGravity(btVector3(0, 0, 0)); // Sin gravedad
            rb->GetRigidBody()->setDamping(0.9f, 0.9f);         // Frenado rápido (evita deslizamiento)
            rb->GetRigidBody()->setAngularFactor(btVector3(0, 0, 0)); // Evita que la cámara rote al chocar
        }
        rb->Start(); // Inicializar física
    }

    // 2. COLLIDER (Esfera)
    ComponentSphereCollider* col = (ComponentSphereCollider*)editorCameraGO->CreateComponent(ComponentType::COLLIDER_SPHERE);
    if (col)
    {
        col->SetRadius(0.5f); // Tamaño de la colisión de la cámara
    }    
    Application& app = Application::GetInstance();
    int width, height;
    app.window->GetWindowSize(width, height);

    if (editorCamera && height > 0)
    {
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        editorCamera->SetAspectRatio(aspectRatio);
    }

    return true;
}

bool ModuleCamera::Update()
{
    Application& app = Application::GetInstance();

    int width, height;
    app.window->GetWindowSize(width, height);

    ComponentCamera* activeCamera = GetActiveCamera();
    if (activeCamera && height > 0)
    {
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        if (aspectRatio != activeCamera->GetAspectRatio())
        {
            activeCamera->SetAspectRatio(aspectRatio);
        }
    }

    if (useEditorCamera)
    {
        HandleEditorControls();
    }

    // Update active camera
    if (activeCamera)
    {
        activeCamera->Update();
    }

	// Update scene camera if different from active
    if (sceneCamera && sceneCamera != activeCamera && sceneCamera->owner)
    {
        sceneCamera->Update();
    }

    return true;
}

bool ModuleCamera::CleanUp()
{
    LOG_DEBUG("Cleaning up Camera Module");

    if (editorCameraGO)
    {
        delete editorCameraGO;
        editorCameraGO = nullptr;
        editorCamera = nullptr;
    }

    sceneCamera = nullptr;

    return true;
}

ComponentCamera* ModuleCamera::GetActiveCamera() const
{
    if (useEditorCamera)
    {
        return editorCamera;
    }
    else
    {
        return sceneCamera;
    }
}

void ModuleCamera::HandleEditorControls()
{
    if (!editorCamera)
        return;

    Application& app = Application::GetInstance();

    // Mouse wheel for zoom (scroll)
    // This is already handled in Input.cpp

    // Right mouse button - Handled in Input.cpp
    // Middle mouse button - Handled in Input.cpp
    // F key - Focus - Handled in Input.cpp
}