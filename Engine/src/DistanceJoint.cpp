#include "DistanceJoint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"

DistanceJoint::DistanceJoint(GameObject* owner) : Joint(owner)
{
    name = "Distance Joint";
    type = ComponentType::DISTANCE_JOINT;
    RefreshJoint();
}

DistanceJoint::~DistanceJoint() {}

void DistanceJoint::CreateJoint() {

    auto* physics = Application::GetInstance().physics->GetPhysics();
    if (!physics) return;
    if (!bodyA || !bodyA->GetActor()) return;

    bool isADynamic = (bodyA->GetBodyType() == Rigidbody::Type::DYNAMIC);
    bool isBDynamic = (bodyB != nullptr) && (bodyB->GetBodyType() == Rigidbody::Type::DYNAMIC);

    if (!isADynamic && !isBDynamic) return;

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

    pxJoint = physx::PxDistanceJointCreate(*physics, actorA, localA, actorB, localB);
    if (pxJoint == nullptr) return;

    pxJoint->setBreakForce(breakForce, breakTorque);

    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    dJoint->setMaxDistance(maxDistance);
    dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, maxDistanceEnabled);
    dJoint->setMinDistance(minDistance);
    dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, minDistance > 0.0f);
    dJoint->setStiffness(stiffness);
    dJoint->setDamping(damping);
    dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eSPRING_ENABLED, springEnabled);
}

void DistanceJoint::EnableSpring(bool b)
{
    springEnabled = b;
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eSPRING_ENABLED, springEnabled);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::EnableMaxDistance(bool b)
{
    maxDistanceEnabled = b;
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, maxDistanceEnabled);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::SetDamping(float d)
{
    damping = glm::clamp(d, 0.0f, INFINITY);
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setDamping(damping);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::SetStiffness(float s)
{
    stiffness = glm::clamp(s, 0.0f, INFINITY);
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setStiffness(stiffness);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::SetMaxDistance(float m)
{
    maxDistance = glm::clamp(m, 0.0f, INFINITY);
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setMaxDistance(maxDistance);
        dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, maxDistanceEnabled);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::SetMinDistance(float m)
{
    minDistance = glm::clamp(m, 0.0f, maxDistance);
    auto* dJoint = static_cast<physx::PxDistanceJoint*>(pxJoint);
    if (dJoint) {
        dJoint->setMinDistance(minDistance);
        dJoint->setDistanceJointFlag(physx::PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, minDistance > 0.0f);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void DistanceJoint::DrawDebug()
{
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

    float currentDist = glm::distance(pA, pB);
    glm::vec4 color = (maxDistanceEnabled && currentDist >= maxDistance * 0.95f)
        ? glm::vec4(1, 0, 0, 1) : glm::vec4(1, 1, 1, 1);

    auto* render = Application::GetInstance().renderer.get();
    render->DrawLine(pA, pB, color);
    render->DrawSphere(pA, 0.2f, glm::vec4(1, 1, 1, 1));
    render->DrawSphere(pB, 0.2f, glm::vec4(1, 1, 1, 1));
}