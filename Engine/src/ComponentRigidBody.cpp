#include "ComponentRigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "ModulePhysics.h"

#include <glm/gtc/type_ptr.hpp>

ComponentRigidBody::ComponentRigidBody(GameObject* owner) : Component(owner, ComponentType::RIGIDBODY)
{
}

ComponentRigidBody::~ComponentRigidBody()
{
    RemoveBodyFromWorld();
    
    if (rigidBody) delete rigidBody;
    if (motionState) delete motionState;
    if (colShape) delete colShape;
}

void ComponentRigidBody::Start()
{
    // 1. Obtener Transform del GameObject
    Transform* trans = dynamic_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 pos = trans->GetPosition();
    glm::quat rot = trans->GetRotation();
    glm::vec3 scale = trans->GetScale(); // <--- LEEMOS LA ESCALA

    // 2. Crear Shape: CAJA
    // Primitives::CreateCube crea un cubo de 1x1x1 (de -0.5 a 0.5).
    // btBoxShape toma "half-extents" (mitad del tamaño desde el centro).
    // Por tanto: (1.0 * scale) / 2.0 = scale * 0.5
    colShape = new btBoxShape(btVector3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));

    // 3. Configurar posición inicial
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    startTransform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    motionState = new btDefaultMotionState(startTransform);

    // 4. Calcular inercia (solo si es dinámico)
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f)
        colShape->calculateLocalInertia(mass, localInertia);

    // 5. Crear cuerpo rígido
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, colShape, localInertia);
    rigidBody = new btRigidBody(rbInfo);

    // 6. Añadir al mundo
    AddBodyToWorld();
}

void ComponentRigidBody::Update()
{
    // Solo actualizamos si el objeto es dinámico (se mueve)
    if (mass > 0.0f && rigidBody)
    {
        // Obtener la transformación simulada por Bullet
        btTransform trans;
        if (rigidBody->getMotionState())
        {
            rigidBody->getMotionState()->getWorldTransform(trans);
        }
        else
        {
            trans = rigidBody->getWorldTransform();
        }

        // Convertir de Bullet a GLM
        btVector3 origin = trans.getOrigin();
        btQuaternion rotation = trans.getRotation();

        glm::vec3 newPos(origin.getX(), origin.getY(), origin.getZ());
        glm::quat newRot(rotation.getW(), rotation.getX(), rotation.getY(), rotation.getZ());

        // Actualizar el Transform del GameObject
        Transform* transformComp = dynamic_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        if (transformComp)
        {
            transformComp->SetPosition(newPos);
            // CORRECCIÓN: Usamos SetRotationQuat en lugar de SetRotation
            transformComp->SetRotationQuat(newRot); 
        }
    }
}

void ComponentRigidBody::SetMass(float newMass)
{
    mass = newMass;
}

void ComponentRigidBody::AddBodyToWorld()
{
    if (rigidBody && Application::GetInstance().physics)
    {
        Application::GetInstance().physics->GetWorld()->addRigidBody(rigidBody);
    }
}

void ComponentRigidBody::RemoveBodyFromWorld()
{
    if (rigidBody && Application::GetInstance().physics)
    {
        Application::GetInstance().physics->GetWorld()->removeRigidBody(rigidBody);
    }
}