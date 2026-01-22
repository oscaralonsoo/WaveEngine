#include "ComponentRigidBody.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "ModulePhysics.h"

#include <imgui.h> 
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

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
        rigidBody = nullptr;
    }
    
    if (colShape) {
        delete colShape;
        colShape = nullptr;
    }
}

void ComponentRigidBody::Start()
{
    Transform* trans = dynamic_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!trans) return;

    lastScale = trans->GetScale();
    CreateShape();

    // 1. Obtenemos Transformación GLOBAL (Mundo)
    glm::mat4 globalMat = trans->GetGlobalMatrix();
    glm::vec3 worldPos = glm::vec3(globalMat[3]);
    glm::quat worldRot = glm::quat_cast(globalMat);

    // 2. Aplicamos el offset (Visual -> Física)
    glm::vec3 physWorldPos = worldPos + (worldRot * centerOffset);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(physWorldPos.x, physWorldPos.y, physWorldPos.z));
    startTransform.setRotation(btQuaternion(worldRot.x, worldRot.y, worldRot.z, worldRot.w));

    // 3. Crear el cuerpo
    motionState = new btDefaultMotionState(startTransform);
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.0f) colShape->calculateLocalInertia(mass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, colShape, localInertia);
    rigidBody = new btRigidBody(rbInfo);

    AddBodyToWorld();
    UpdateRigidBodyScale();
    
    if (rigidBody) rigidBody->activate(true);
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

void ComponentRigidBody::Update()
{
    // Solo si el juego está corriendo, la física manda sobre el transform
    if (Application::GetInstance().GetPlayState() == Application::PlayState::PLAYING)
    {
        if (rigidBody && rigidBody->getMotionState())
        {
            btTransform btTrans;
            rigidBody->getMotionState()->getWorldTransform(btTrans);

            Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
            if (trans)
            {
                // Convertimos la posición de Bullet (matriz) de vuelta a nuestro Transform
                float matrix[16];
                btTrans.getOpenGLMatrix(matrix);
                
                // Suponiendo que tienes un método para setear la matriz global o posición
                glm::mat4 glmMat = glm::make_mat4(matrix);
                trans->SetGlobalPosition(glm::vec3(glmMat[3]));
                trans->SetGlobalRotationQuat(glm::quat_cast(glmMat));
            }
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
        btVector3 scale(lastScale.x, lastScale.y, lastScale.z);
        colShape->setLocalScaling(scale);
        
        if (mass > 0.0f) {
            btVector3 localInertia(0, 0, 0);
            colShape->calculateLocalInertia(mass, localInertia);
            rigidBody->setMassProps(mass, localInertia);
        }
        
        rigidBody->activate(true);
        
        // ¡Seguridad extra aquí también!
        if (Application::GetInstance().physics && 
            Application::GetInstance().physics->GetWorld() != nullptr) 
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
        btTransform btTrans;
        // Convertimos la mat4 de GLM a la btTransform de Bullet
        btTrans.setFromOpenGLMatrix(glm::value_ptr(trans->GetGlobalMatrix()));

        // 1. Actualizamos el transform del cuerpo
        rigidBody->setWorldTransform(btTrans);

        // 2. Sincronizamos el MotionState (para que no haya saltos al dar al Play)
        if (rigidBody->getMotionState())
        {
            rigidBody->getMotionState()->setWorldTransform(btTrans);
        }

        // 3. Forzamos a Bullet a actualizar el volumen de colisión (AABB) 
        // para que el Debug Drawer se mueva en el editor
        if (Application::GetInstance().physics->GetWorld())
        {
            Application::GetInstance().physics->GetWorld()->updateSingleAabb(rigidBody);
        }
    }
}