#include "ComponentRigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "ModulePhysics.h"

#include <imgui.h> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include "ComponentCollider.h"

ComponentRigidBody::ComponentRigidBody(GameObject* owner) 
    : Component(owner, ComponentType::RIGIDBODY), 
      centerOffset(0.0f, 0.0f, 0.0f),
      lastScale(1.0f, 1.0f, 1.0f),
      shapeType(ShapeType::BOX) // Inicializamos en BOX
{
}

ComponentRigidBody::~ComponentRigidBody()
{
    RemoveBodyFromWorld();
    
    if (rigidBody) { 
        delete rigidBody->getMotionState(); 
        delete rigidBody; 
    }

}

void ComponentRigidBody::Start()
{
    Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans) {
        initialPos = trans->GetPosition();
        initialRot = trans->GetRotationQuat();
        initialScale = trans->GetScale();
    }

    // 1. Buscamos un Collider (Box, Sphere, etc.)
    // Primero intentamos con BOX, luego con SPHERE (según los tipos que hayas creado)
    ComponentCollider* collider = (ComponentCollider*)owner->GetComponent(ComponentType::COLLIDER_BOX);
    
    if (collider == nullptr) {
        collider = (ComponentCollider*)owner->GetComponent(ComponentType::COLLIDER_SPHERE);
    }

    // 2. Asignamos la forma
    if (collider != nullptr) {
        colShape = collider->GetShape(); 
        // Nota: El Rigidbody NO debe borrar colShape en su destructor si pertenece al Collider
    } else {
        // Fallback: Caja de 1x1x1 por si olvidamos poner un collider
        colShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    }

    // 3. Calculamos la inercia (si tiene masa, reacciona a fuerzas)
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f) {
        colShape->calculateLocalInertia(mass, localInertia);
    }

    // 4. Sincronizamos con el Transform global actual
    btTransform startTransform;
    startTransform.setFromOpenGLMatrix(glm::value_ptr(trans->GetGlobalMatrix()));

    // 5. Creamos el MotionState y el RigidBody
    motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, colShape, localInertia);
    
    rigidBody = new btRigidBody(rbInfo);

    // 6. Añadimos el cuerpo al mundo de Bullet
    AddBodyToWorld();
}

// Nueva función para gestionar la creación de formas
void ComponentRigidBody::CreateShape()
{
    if (colShape) {
        delete colShape;
        colShape = nullptr;
    }

    switch (shapeType)
    {
    case ShapeType::BOX:
        // Caja base de 1x1x1 (Half extents 0.5)
        colShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        break;

    case ShapeType::SPHERE:
        // Esfera base de radio 0.5 (Diámetro 1.0)
        colShape = new btSphereShape(0.5f);
        break;

    case ShapeType::CAPSULE:
        // Cápsula eje Y: radio 0.5, altura 1.0 (altura total = 2.0 contando semiesferas)
        // Bullet define altura como la parte cilíndrica.
        colShape = new btCapsuleShape(0.5f, 1.0f);
        break;

    case ShapeType::CYLINDER:
        // Cilindro eje Y: radio 0.5, altura 1.0 (half extent 0.5)
        colShape = new btCylinderShape(btVector3(0.5f, 0.5f, 0.5f));
        break;
    }
}

void ComponentRigidBody::Update() {
    if (Application::GetInstance().GetPlayState() == Application::PlayState::PLAYING) {
        if (rigidBody && rigidBody->getMotionState()) {
            btTransform btTrans;
            rigidBody->getMotionState()->getWorldTransform(btTrans);

            // Pasamos la posición de Bullet a nuestro Transform visual
            Transform* trans = (Transform*)owner->GetComponent(ComponentType::TRANSFORM);
            
            btVector3 pos = btTrans.getOrigin();
            btQuaternion rot = btTrans.getRotation();

            trans->SetPosition(glm::vec3(pos.x(), pos.y(), pos.z()));
            trans->SetRotationQuat(glm::quat(rot.w(), rot.x(), rot.y(), rot.z()));
        }
    }
}

void ComponentRigidBody::OnEditor()
{
    if (ImGui::CollapsingHeader("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- SELECTOR DE FORMA ---
        const char* shapeItems[] = { "Box", "Sphere", "Capsule", "Cylinder" };
        int currentItem = static_cast<int>(shapeType);
        if (ImGui::Combo("Shape", &currentItem, shapeItems, IM_ARRAYSIZE(shapeItems)))
        {
            SetShapeType(static_cast<ShapeType>(currentItem));
        }
        // -------------------------

        float newMass = mass;
        if (ImGui::DragFloat("Mass", &newMass, 0.1f, 0.0f, 1000.0f))
        {
            SetMass(newMass);
        }

        glm::vec3 newOffset = centerOffset;
        if (ImGui::DragFloat3("Center Offset", (float*)&newOffset, 0.1f))
        {
            SetCenterOffset(newOffset);
        }

        SyncToBullet();
    }
}

void ComponentRigidBody::SetShapeType(ShapeType newType)
{
    if (shapeType != newType)
    {
        shapeType = newType;
        // Si el cuerpo ya existe, hay que recrearlo
        if (rigidBody)
        {
            RemoveBodyFromWorld();
            CreateShape(); // Crea el nuevo btCollisionShape
            
            // Reasignar shape al rigidBody existente
            rigidBody->setCollisionShape(colShape);
            
            // Recalcular inercia y propiedades
            UpdateRigidBodyScale(); // Esto recalcula inercia y aplica escala
            
            AddBodyToWorld();
        }
    }
}

void ComponentRigidBody::SetMass(float newMass)
{
    if (mass != newMass)
    {
        mass = newMass;
        if (rigidBody)
        {
            RemoveBodyFromWorld();
            
            btVector3 localInertia(0, 0, 0);
            if (mass > 0.0f)
                colShape->calculateLocalInertia(mass, localInertia);
            
            rigidBody->setMassProps(mass, localInertia);
            rigidBody->clearForces();
            
            AddBodyToWorld();
        }
    }
}

void ComponentRigidBody::SetCenterOffset(const glm::vec3& offset)
{
    centerOffset = offset;
}

void ComponentRigidBody::AddBodyToWorld()
{
    // Añadimos comprobación triple: que exista el módulo, que exista el mundo y que tengamos un rigidBody
    if (rigidBody && 
        Application::GetInstance().physics && 
        Application::GetInstance().physics->GetWorld() != nullptr)
    {
        Application::GetInstance().physics->GetWorld()->addRigidBody(rigidBody);
    }
    else {
        // Esto te avisará en la consola si algo falla en lugar de cerrarse
        LOG_CONSOLE("Physics Warning: No se pudo añadir el cuerpo al mundo (Mundo no inicializado)");
    }
}

void ComponentRigidBody::UpdateRigidBodyScale()
{
    if (rigidBody && colShape)
    {
        // Aplicamos la escala del Transform a la forma de Bullet
        btVector3 scale(lastScale.x, lastScale.y, lastScale.z);
        colShape->setLocalScaling(scale);
        
        // Es vital recalcular la inercia, ya que un objeto más grande es más difícil de mover
        if (mass > 0.0f) {
            btVector3 localInertia(0, 0, 0);
            colShape->calculateLocalInertia(mass, localInertia);
            rigidBody->setMassProps(mass, localInertia);
        }
        
        // Forzamos a Bullet a reevaluar el objeto
        rigidBody->activate(true);
        
        if (Application::GetInstance().physics && Application::GetInstance().physics->GetWorld()) 
        {
            Application::GetInstance().physics->GetWorld()->updateSingleAabb(rigidBody);
        }
    }
}

void ComponentRigidBody::RemoveBodyFromWorld()
{
    if (rigidBody && Application::GetInstance().physics)
    {
        Application::GetInstance().physics->GetWorld()->removeRigidBody(rigidBody);
    }
}

void ComponentRigidBody::SyncToBullet()
{
    Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans && rigidBody)
    {
        glm::mat4 globalMat = trans->GetGlobalMatrix();

        // 1. EXTRAER POSICIÓN
        btVector3 pos(globalMat[3][0], globalMat[3][1], globalMat[3][2]);

        // 2. EXTRAER ROTACIÓN LIMPIA (Normalizando los ejes para eliminar la escala de la matriz)
        glm::vec3 axisX = glm::normalize(glm::vec3(globalMat[0]));
        glm::vec3 axisY = glm::normalize(glm::vec3(globalMat[1]));
        glm::vec3 axisZ = glm::normalize(glm::vec3(globalMat[2]));

        btMatrix3x3 basis(
            axisX.x, axisY.x, axisZ.x,
            axisX.y, axisY.y, axisZ.y,
            axisX.z, axisY.z, axisZ.z
        );

        btTransform btTrans(basis, pos);

        // Aplicamos a Bullet el transform SIN ESCALA
        rigidBody->setWorldTransform(btTrans);
        if (rigidBody->getMotionState())
            rigidBody->getMotionState()->setWorldTransform(btTrans);

        // 3. EXTRAER ESCALA GLOBAL PARA EL SHAPE
        // La escala se calcula con la longitud de los ejes de la matriz
        glm::vec3 currentScale(
            glm::length(glm::vec3(globalMat[0])),
            glm::length(glm::vec3(globalMat[1])),
            glm::length(glm::vec3(globalMat[2]))
        );

        // Solo actualizamos si ha cambiado significativamente
        if (glm::distance(currentScale, lastScale) > 0.001f) 
        {
            lastScale = currentScale;
            UpdateRigidBodyScale();
        }
    }
}

void ComponentRigidBody::ResetPhysics()
{
    if (!rigidBody) return;

    // 1. Devolver los valores al Transform visual
    Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans) {
        trans->SetPosition(initialPos);
        trans->SetRotationQuat(initialRot);
        trans->SetScale(initialScale);
    }

    // 2. Sincronizar Bullet con la posición inicial
    btTransform btTrans;
    btTrans.setIdentity();
    btTrans.setOrigin(btVector3(initialPos.x, initialPos.y, initialPos.z));
    btTrans.setRotation(btQuaternion(initialRot.x, initialRot.y, initialRot.z, initialRot.w));
    
    rigidBody->setWorldTransform(btTrans);
    if (rigidBody->getMotionState()) {
        rigidBody->getMotionState()->setWorldTransform(btTrans);
    }

    // 3. LIMPIAR FUERZAS Y VELOCIDADES (Esto arregla que no caigan)
    rigidBody->setLinearVelocity(btVector3(0, 0, 0));
    rigidBody->setAngularVelocity(btVector3(0, 0, 0));
    rigidBody->clearForces();
    
    // 4. DESPERTAR AL CUERPO (Warping/Reactivating)
    rigidBody->activate(true);
}