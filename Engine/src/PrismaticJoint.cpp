#include "PrismaticJoint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"
#include "imgui.h"

PrismaticJoint::PrismaticJoint(GameObject* owner) : Joint(owner) {
    name = "Prismatic Joint";
    type = ComponentType::PRISMATIC_JOINT;
    RefreshJoint();
}

PrismaticJoint::~PrismaticJoint() {}

void PrismaticJoint::CreateJoint() {
    auto* physics = Application::GetInstance().physics->GetPhysics();
   
    if (!physics || !bodyA || !bodyA->GetActor()) return;

    bool isADynamic = (bodyA->GetBodyType() == Rigidbody::Type::DYNAMIC);

    bool isBDynamic = (bodyB != nullptr) && (bodyB->GetBodyType() == Rigidbody::Type::DYNAMIC);

    if (!isADynamic && !isBDynamic) {
        /*LOG(LogType::LOG_WARNING, "Prismatic Joint ignored: At least one body must be DYNAMIC.");*/
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

    pxJoint = physx::PxPrismaticJointCreate(*physics, actorA, localA, actorB, localB);
    if (!pxJoint) return;

    pxJoint->setBreakForce(breakForce, breakTorque);

    auto* pJoint = static_cast<physx::PxPrismaticJoint*>(pxJoint);

    if (pxJoint == nullptr) {
        /*LOG(LogType::LOG_ERROR, "Joint Error: PhysX failed to create PxPrismaticJoint");*/
        return;
    }

    physx::PxTolerancesScale scale;
    physx::PxJointLinearLimitPair limit(scale, minLimit, maxLimit);

    if (softLimitEnabled) {
        limit.stiffness = stiffness;
        limit.damping = damping;
    }

    pJoint->setLimit(limit);
    pJoint->setPrismaticJointFlag(physx::PxPrismaticJointFlag::eLIMIT_ENABLED, limitsEnabled);
}

void PrismaticJoint::EnableLimits(bool b) {
    limitsEnabled = b;
    auto* pJoint = static_cast<physx::PxPrismaticJoint*>(pxJoint);
    if (pJoint) {
        pJoint->setPrismaticJointFlag(physx::PxPrismaticJointFlag::eLIMIT_ENABLED, limitsEnabled);
        if (bodyA) bodyA->WakeUp();
    }
}

void PrismaticJoint::EnableSoftLimit(bool b) {
    softLimitEnabled = b;
    auto* pJoint = static_cast<physx::PxPrismaticJoint*>(pxJoint);
    if (pJoint) {

        physx::PxTolerancesScale scale;
        physx::PxJointLinearLimitPair limit(scale, minLimit, maxLimit);

        if (softLimitEnabled) {
            limit.stiffness = stiffness;
            limit.damping = damping;
        }

        pJoint->setLimit(limit);

        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void PrismaticJoint::SetStiffness(float s) {
    stiffness = glm::max(0.0f, s);
    if (softLimitEnabled) {
        EnableSoftLimit(true);
    }
}

void PrismaticJoint::SetDamping(float d) {
    damping = glm::max(0.0f, d);
    if (softLimitEnabled) {
        EnableSoftLimit(true);
    }
}


void PrismaticJoint::SetMinLimit(float m) {
    minLimit = m;
    auto* pJoint = static_cast<physx::PxPrismaticJoint*>(pxJoint);
    if (pJoint) {
        physx::PxTolerancesScale scale;
        physx::PxJointLinearLimitPair limit(scale, minLimit, maxLimit);
        limit.stiffness = stiffness;
        limit.damping = damping;
        pJoint->setLimit(limit);
        if (bodyA) bodyA->WakeUp();
    }
}

void PrismaticJoint::SetMaxLimit(float m) {
    maxLimit = m;
    auto* pJoint = static_cast<physx::PxPrismaticJoint*>(pxJoint);
    if (pJoint) {
        physx::PxTolerancesScale scale;
        physx::PxJointLinearLimitPair limit(scale, minLimit, maxLimit);
        limit.stiffness = stiffness;
        limit.damping = damping;
        pJoint->setLimit(limit);
        if (bodyA) bodyA->WakeUp();
    }
}

//void PrismaticJoint::Save(Config& config) {
//    SaveBase(config);
//    config.SetBool("LimitsEnabled", limitsEnabled);
//    config.SetFloat("MinLimit", minLimit);
//    config.SetFloat("MaxLimit", maxLimit);
//    config.SetBool("SoftLimit", softLimitEnabled);
//    config.SetFloat("Stiffness", stiffness);
//    config.SetFloat("Damping", damping);
//}
//
//void PrismaticJoint::Load(Config& config) {
//    LoadBase(config);
//    EnableLimits(config.GetBool("LimitsEnabled"));
//    SetMinLimit(config.GetFloat("MinLimit", -5.0f));
//    SetMaxLimit(config.GetFloat("MaxLimit", 5.0f));
//    EnableSoftLimit(config.GetBool("SoftLimit"));
//    SetStiffness(config.GetFloat("Stiffness"));
//    SetDamping(config.GetFloat("Damping"));
//}

void PrismaticJoint::OnEditor() {
    OnEditorBase();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Limit Settings", flags)) {
        if (ImGui::Checkbox("Enable Limits", &limitsEnabled)) EnableLimits(limitsEnabled);
        if (ImGui::InputFloat("Min Limit", &minLimit)) SetMinLimit(minLimit);
        if (ImGui::InputFloat("Max Limit", &maxLimit)) SetMaxLimit(maxLimit);
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Soft Limit (Spring)", flags)) {
        if (ImGui::Checkbox("Enable Soft Limit", &softLimitEnabled)) RefreshJoint();
        ImGui::InputFloat("Stiffness", &stiffness);
        ImGui::InputFloat("Damping", &damping);
        if (ImGui::IsItemDeactivatedAfterEdit()) RefreshJoint();
        ImGui::TreePop();
    }
}

void PrismaticJoint::DrawDebug() {
    if (!bodyA || !pxJoint) return;

    physx::PxTransform poseRef = (bodyB) ? bodyB->GetActor()->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
    physx::PxTransform localRef = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
    physx::PxTransform worldRef = poseRef.transform(localRef);

    glm::vec3 pRef(worldRef.p.x, worldRef.p.y, worldRef.p.z);
    physx::PxVec3 bX = worldRef.q.getBasisVector0();
    glm::vec3 dirX(bX.x, bX.y, bX.z);

    auto* render = Application::GetInstance().renderer.get();
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    render->DrawLine(pRef - dirX * 10.0f, pRef + dirX * 10.0f, color);

    if (limitsEnabled) {
        glm::vec3 pMin = pRef + dirX * minLimit;
        glm::vec3 pMax = pRef + dirX * maxLimit;
        render->DrawLine(pMin, pMax, color);
        render->DrawSphere(pMin, 0.1f, color);
        render->DrawSphere(pMax, 0.1f, color);
    }

    physx::PxTransform poseA = bodyA->GetActor()->getGlobalPose();
    physx::PxTransform localA = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
    physx::PxTransform worldA = poseA.transform(localA);
    render->DrawSphere(glm::vec3(worldA.p.x, worldA.p.y, worldA.p.z), 0.2f, color);
}