#include "FixedJoint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"
#include "imgui.h"

FixedJoint::FixedJoint(GameObject* owner) : Joint(owner)
{
    name = "Fixed Joint";
    type = ComponentType::FIXED_JOINT;
    RefreshJoint();
}

FixedJoint::~FixedJoint() {}

void FixedJoint::CreateJoint() {
    
    auto* physics = Application::GetInstance().physics->GetPhysics();

    if (!physics) {
        /*LOG(LogType::LOG_ERROR, "Joint Error: PhysX Physics pointer is NULL");*/
        return;
    }

    if (!bodyA || !bodyA->GetActor()) {
        /*LOG(LogType::LOG_ERROR, "Joint Error: BodyA or its PhysX Actor is NULL");*/
        return;
    }


    bool isADynamic = (bodyA->GetBodyType() == Rigidbody::Type::DYNAMIC);

    bool isBDynamic = (bodyB != nullptr) && (bodyB->GetBodyType() == Rigidbody::Type::DYNAMIC);

    if (!isADynamic && !isBDynamic) {
        /*LOG(LogType::LOG_WARNING, "Fixed Joint ignored: At least one body must be DYNAMIC.");*/
        return;
    }

    physx::PxRigidActor* actorA = bodyA->GetActor();
    physx::PxRigidActor* actorB = (bodyB) ? bodyB->GetActor() : nullptr;

    physx::PxTransform localA(
        physx::PxVec3(localPosA.x, localPosA.y, localPosA.z),
        physx::PxQuat(localRotA.x, localRotA.y, localRotA.z, localRotA.w)
    );

    physx::PxTransform localB(
        physx::PxVec3(localPosB.x, localPosB.y, localPosB.z),
        physx::PxQuat(localRotB.x, localRotB.y, localRotB.z, localRotB.w)
    );

    pxJoint = physx::PxFixedJointCreate(*physics, actorA, localA, actorB, localB);

    if (pxJoint == nullptr) {
        /*LOG(LogType::LOG_ERROR, "Joint Error: PhysX failed to create PxFixedJoint");*/
        return;
    }

    pxJoint->setBreakForce(breakForce, breakTorque);
}

//void FixedJoint::Save(Config& config) {
//    SaveBase(config);
//}
//
//void FixedJoint::Load(Config& config) {
//    LoadBase(config);
//}

void FixedJoint::OnEditor() {
    
    OnEditorBase();
}

void FixedJoint::DrawDebug() {
    
    if (!bodyA || !pxJoint) return;

    physx::PxTransform poseA = bodyA->GetActor()->getGlobalPose();
    physx::PxTransform localA = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
    physx::PxTransform worldA = poseA.transform(localA);
    glm::vec3 pA(worldA.p.x, worldA.p.y, worldA.p.z);

    glm::vec3 pB;
    if (bodyB) {
        physx::PxTransform poseB = bodyB->GetActor()->getGlobalPose();
        physx::PxTransform localB = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
        physx::PxTransform worldB = poseB.transform(localB);
        pB = glm::vec3(worldB.p.x, worldB.p.y, worldB.p.z);
    }
    else {
        physx::PxTransform worldB = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
        pB = glm::vec3(worldB.p.x, worldB.p.y, worldB.p.z);
    }

    auto* render = Application::GetInstance().renderer.get();

    glm::vec4 color = glm::vec4(1, 1, 1, 1);

    render->DrawLine(pA, pB, color);
    render->DrawSphere(pA, 0.2f, color);
    render->DrawSphere(pB, 0.2f, color);
}