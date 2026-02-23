#include "SphericalJoint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"

SphericalJoint::SphericalJoint(GameObject* owner) : Joint(owner)
{
    name = "Spherical Joint";
    type = ComponentType::SPHERICAL_JOINT;
    RefreshJoint();
}

SphericalJoint::~SphericalJoint() {}

void SphericalJoint::CreateJoint() {
    auto* physics = Application::GetInstance().physics->GetPhysics();
    if (!physics || !bodyA || !bodyA->GetActor()) return;

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

    pxJoint = physx::PxSphericalJointCreate(*physics, actorA, localA, actorB, localB);
    if (pxJoint == nullptr) return;

    pxJoint->setBreakForce(breakForce, breakTorque);

    auto* sJoint = static_cast<physx::PxSphericalJoint*>(pxJoint);
    physx::PxJointLimitCone limit(glm::radians(limitAngle), glm::radians(limitAngle));
    sJoint->setLimitCone(limit);
    sJoint->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, limitsEnabled);
}

void SphericalJoint::EnableLimits(bool b) {
    limitsEnabled = b;
    auto* sJoint = static_cast<physx::PxSphericalJoint*>(pxJoint);
    if (sJoint) {
        sJoint->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, limitsEnabled);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void SphericalJoint::SetConeLimit(float angle) {
    limitAngle = glm::clamp(angle, 0.01f, 179.9f);
    auto* sJoint = static_cast<physx::PxSphericalJoint*>(pxJoint);
    if (sJoint) {
        physx::PxJointLimitCone limit(glm::radians(limitAngle), glm::radians(limitAngle));
        sJoint->setLimitCone(limit);
        if (bodyA) bodyA->WakeUp();
    }
}

void SphericalJoint::DrawDebug() {
    if (!bodyA || !pxJoint) return;

    physx::PxTransform poseRef = (bodyB) ? bodyB->GetActor()->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
    physx::PxTransform localRef = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
    physx::PxTransform worldRef = poseRef.transform(localRef);

    glm::vec3 pRef(worldRef.p.x, worldRef.p.y, worldRef.p.z);
    physx::PxVec3 bX = worldRef.q.getBasisVector0();
    physx::PxVec3 bY = worldRef.q.getBasisVector1();
    physx::PxVec3 bZ = worldRef.q.getBasisVector2();

    glm::vec3 dirX(bX.x, bX.y, bX.z);
    glm::vec3 dirY(bY.x, bY.y, bY.z);
    glm::vec3 dirZ(bZ.x, bZ.y, bZ.z);

    auto* render = Application::GetInstance().renderer.get();
    float radius = 1.2f;
    glm::vec4 color = glm::vec4(1, 1, 1, 1);

    render->DrawSphere(pRef, 0.05f, color);

    if (limitsEnabled) {
        const int segments = 32;
        float rad = glm::radians(limitAngle);

        glm::vec3 prevPoint;
        for (int i = 0; i <= segments; ++i) {
            float theta = (i * 2.0f * 3.14159265f) / segments;
            glm::vec3 currentPoint = pRef + (dirX * cosf(rad) +
                (dirY * cosf(theta) + dirZ * sinf(theta)) * sinf(rad)) * radius;
            if (i > 0) render->DrawLine(prevPoint, currentPoint, color);
            if (i % (segments / 4) == 0) render->DrawLine(pRef, currentPoint, color * 0.4f);
            prevPoint = currentPoint;
        }
    }

    physx::PxTransform poseA = bodyA->GetActor()->getGlobalPose();
    physx::PxTransform worldA = poseA.transform(pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0));
    physx::PxVec3 bXA = worldA.q.getBasisVector0();
    render->DrawLine(pRef, pRef + glm::vec3(bXA.x, bXA.y, bXA.z) * radius, glm::vec4(1, 1, 1, 1));
}