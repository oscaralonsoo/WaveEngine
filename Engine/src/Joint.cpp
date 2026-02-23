#include "Joint.h"
#include "Rigidbody.h"
#include "GameObject.h"
#include "imgui.h"

#include "Application.h"
#include "ModuleScene.h"
#include <glm/gtc/quaternion.hpp>

glm::vec3 QuatToEuler(const glm::quat& quaternion) {
    return glm::degrees(glm::eulerAngles(quaternion));
}

glm::quat EulerToQuat(const glm::vec3& eulerDegrees) {
    glm::vec3 radians = glm::radians(eulerDegrees);
    return glm::quat(radians);
}

Joint::Joint(GameObject* owner) : Component(owner, ComponentType::JOINT) {

    localPosA = glm::vec3(0.0f);
    localPosB = glm::vec3(0.0f);
    localRotA = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    localRotB = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    bodyB = nullptr;

    bodyA = (Rigidbody*)owner->GetComponent(ComponentType::RIGIDBODY);
    if (bodyA) {
        bodyA->RegisterJoint(this);
    }
}

void Joint::Update()
{
    DrawDebug();
}

void Joint::CleanUp()
{
    DestroyJoint();

    if (bodyA) {
        bodyA->UnregisterJoint(this);
    }
    if (bodyB) {
        bodyB->UnregisterJoint(this);
    }

    bodyA = nullptr;
    bodyB = nullptr;
}

void Joint::RefreshJoint() {
    DestroyJoint();
    if (bodyA != nullptr) {
        CreateJoint();
    }
}

void Joint::DestroyJoint() {
    if (pxJoint != nullptr) {
        pxJoint->release();
        pxJoint = nullptr;
    }
}

void Joint::SetTarget(GameObject* targetGO) {
    if (bodyB) bodyB->UnregisterJoint(this);
    bodyB = nullptr;

    if (targetGO) {
        bodyB = (Rigidbody*)targetGO->GetComponent(ComponentType::RIGIDBODY);
        if (bodyB) bodyB->RegisterJoint(this);
    }

    RefreshJoint();
}

void Joint::OnRigidbodyReset(Rigidbody* rb) {
    if (rb == nullptr) {
        DestroyJoint();
        return;
    }
    if (rb == bodyA || rb == bodyB) {
        RefreshJoint();
    }
}

void Joint::OnRigidbodyDeleted(Rigidbody* rb) {
    if (rb == bodyA) bodyA = nullptr;
    if (rb == bodyB) bodyB = nullptr;
    DestroyJoint();
}

void Joint::SetAnchorPosition(JointBody bodyToChange, const glm::vec3& position)
{
    if (bodyToChange == JointBody::Self) localPosA = position;
    else localPosB = position;
    SyncFrames();
}

void Joint::SetAnchorRotation(JointBody bodyToChange, const glm::quat& rotation)
{
    if (bodyToChange == JointBody::Self) localRotA = rotation;
    else localRotB = rotation;
    SyncFrames();
}

void Joint::SetBreakForce(float force) {
    breakForce = glm::clamp(force, 0.0f, INFINITY_PHYSIC);
    if (pxJoint) {
        pxJoint->setBreakForce(breakForce, breakTorque);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void Joint::SetBreakTorque(float torque) {
    breakTorque = glm::clamp(torque, 0.0f, INFINITY_PHYSIC);
    if (pxJoint) {
        pxJoint->setBreakForce(breakForce, breakTorque);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void Joint::SyncFrames()
{
    if (!pxJoint) return;

    physx::PxTransform poseA(
        physx::PxVec3(localPosA.x, localPosA.y, localPosA.z),
        physx::PxQuat(localRotA.x, localRotA.y, localRotA.z, localRotA.w)
    );

    physx::PxTransform poseB(
        physx::PxVec3(localPosB.x, localPosB.y, localPosB.z),
        physx::PxQuat(localRotB.x, localRotB.y, localRotB.z, localRotB.w)
    );

    pxJoint->setLocalPose(physx::PxJointActorIndex::eACTOR0, poseA);
    pxJoint->setLocalPose(physx::PxJointActorIndex::eACTOR1, poseB);

    if (bodyA) bodyA->WakeUp();
    if (bodyB) bodyB->WakeUp();
}

void Joint::OnGameObjectEvent(GameObjectEvent event, Component* component)
{
    if (component == nullptr) return;

    switch (event)
    {
    case GameObjectEvent::COMPONENT_ADDED:
        if (component->IsType(ComponentType::RIGIDBODY))
        {
            Rigidbody* rb = (Rigidbody*)component;
            bodyA = rb;
            rb->RegisterJoint(this);
            RefreshJoint();
        }
        break;
    case GameObjectEvent::COMPONENT_REMOVED:
        if (component == (Component*)bodyA)
        {
            DestroyJoint();
            bodyA = nullptr;
        }
        break;
    }
}