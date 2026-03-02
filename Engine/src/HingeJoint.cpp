#include "HingeJoint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"
#include "imgui.h"

HingeJoint::HingeJoint(GameObject* owner) : Joint(owner)
{
    name = "Hinge Joint";
    type = ComponentType::HINGE_JOINT;
    RefreshJoint();
}

HingeJoint::~HingeJoint() {}

void HingeJoint::CreateJoint() {

    auto* physics = Application::GetInstance().physics->GetPhysics();

    if (!physics || !bodyA || !bodyA->GetActor()) return;


    bool isADynamic = (bodyA->GetBodyType() == Rigidbody::Type::DYNAMIC);

    bool isBDynamic = (bodyB != nullptr) && (bodyB->GetBodyType() == Rigidbody::Type::DYNAMIC);

    if (!isADynamic && !isBDynamic) {
        /*LOG(LogType::LOG_WARNING, "Hinge Joint ignored: At least one body must be DYNAMIC.");*/
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

    pxJoint = physx::PxRevoluteJointCreate(*physics, actorA, localA, actorB, localB);

    if (pxJoint == nullptr) {
        /*LOG(LogType::LOG_ERROR, "Joint Error: PhysX failed to create PxHingeJoint");*/
        return;
    }

    pxJoint->setBreakForce(breakForce, breakTorque);

    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);

    physx::PxJointAngularLimitPair limitPair(glm::radians(minAngle), glm::radians(maxAngle));
   
    rJoint->setLimit(limitPair);
    rJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eLIMIT_ENABLED, limitsEnabled);

    rJoint->setDriveVelocity(driveVelocity);
    rJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eDRIVE_ENABLED, motorEnabled);
}

void HingeJoint::EnableLimits(bool b) {
    limitsEnabled = b;
    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
    if (rJoint) {
        rJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eLIMIT_ENABLED, limitsEnabled);
        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void HingeJoint::SetMinAngle(float angle) {
    minAngle = angle;
    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
    if (rJoint) {
        physx::PxJointAngularLimitPair limitPair(glm::radians(minAngle), glm::radians(maxAngle));
        rJoint->setLimit(limitPair);
        if (bodyA) bodyA->WakeUp();
    }
}

void HingeJoint::SetMaxAngle(float angle) {
    maxAngle = angle;
    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
    if (rJoint) {
        physx::PxJointAngularLimitPair limitPair(glm::radians(minAngle), glm::radians(maxAngle));
        rJoint->setLimit(limitPair);
        if (bodyA) bodyA->WakeUp();
    }
}

void HingeJoint::EnableMotor(bool b) {
    motorEnabled = b;
    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
    if (rJoint) {
        rJoint->setRevoluteJointFlag(physx::PxRevoluteJointFlag::eDRIVE_ENABLED, motorEnabled);
        if (bodyA) bodyA->WakeUp();
    }
}

void HingeJoint::SetDriveVelocity(float v) {
    driveVelocity = v;
    auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
    if (rJoint) {
        rJoint->setDriveVelocity(driveVelocity);
        if (bodyA) bodyA->WakeUp();
    }
}

void HingeJoint::Serialize(nlohmann::json& componentObj) const
{
    SerializeBase(componentObj);

    componentObj["Limits"] = {
        {"Enabled", limitsEnabled},
        {"Min", minAngle},
        {"Max", maxAngle}
    };

    componentObj["Motor"] = {
        {"Enabled", motorEnabled},
        {"Velocity", driveVelocity}
    };
}

void HingeJoint::Deserialize(const nlohmann::json& componentObj)
{
    DeserializeBase(componentObj);
    if (componentObj.contains("Limits")) {
        auto limits = componentObj["Limits"];
        EnableLimits(limits.value("Enabled", false));
        SetMinAngle(limits.value("Min", -45.0f));
        SetMaxAngle(limits.value("Max", 45.0f));
    }

    if (componentObj.contains("Motor")) {
        auto motor = componentObj["Motor"];
        EnableMotor(motor.value("Enabled", false));
        SetDriveVelocity(motor.value("Velocity", 0.0f));
    }
    RefreshJoint();
}

void HingeJoint::OnEditor() {
#ifndef WAVE_GAME
    OnEditorBase();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool isNodeOpen = ImGui::TreeNodeEx("Hinge Limits", flags);
    if (isNodeOpen) {
        if (ImGui::Checkbox("Enable Limits", &limitsEnabled)) EnableLimits(limitsEnabled);
        if (ImGui::SliderFloat("Min Angle", &minAngle, -179.9f, 0.0f)) SetMinAngle(minAngle);
        if (ImGui::SliderFloat("Max Angle", &maxAngle, 0.0f, 179.9)) SetMaxAngle(maxAngle);
        ImGui::TreePop();
    }

    isNodeOpen = ImGui::TreeNodeEx("Motor Settings", flags);
    if (isNodeOpen) {
        if (ImGui::Checkbox("Enable Motor", &motorEnabled)) EnableMotor(motorEnabled);
        if (ImGui::InputFloat("Velocity", &driveVelocity)) SetDriveVelocity(driveVelocity);
        ImGui::TreePop();
    }
#endif 
}

void HingeJoint::DrawDebug() 
{
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
    float radius = 1.0f;

    glm::vec4 color = glm::vec4(1, 1, 1, 1);

    render->DrawLine(pRef - dirX * 0.5f, pRef + dirX * 0.5f, color);
    render->DrawSphere(pRef, 0.05f, color);

    if (limitsEnabled) {

        float radMin = glm::radians(minAngle);
        glm::vec3 pMin = pRef + (dirY * cos(radMin) - dirZ * sin(radMin)) * radius;
        render->DrawLine(pRef, pMin, color);

        float radMax = glm::radians(maxAngle);
        glm::vec3 pMax = pRef + (dirY * cos(radMax) - dirZ * sin(radMax)) * radius;
        render->DrawLine(pRef, pMax, color);

        const int segments = 16;
        float step = (radMax - radMin) / segments;
        glm::vec3 prevPoint = pMin;

        for (int i = 1; i <= segments; ++i) {
            float currentRad = radMin + step * i;

            glm::vec3 nextPoint = pRef + (dirY * cos(currentRad) - dirZ * sin(currentRad)) * radius;
            render->DrawLine(prevPoint, nextPoint, color);
            prevPoint = nextPoint;
        }

        physx::PxTransform poseA = bodyA->GetActor()->getGlobalPose();
        physx::PxTransform localA = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0);
        physx::PxTransform worldA = poseA.transform(localA);

        auto* rJoint = static_cast<physx::PxRevoluteJoint*>(pxJoint);
        float currentAngle = rJoint->getAngle();

        glm::vec3 dirNeedle = dirY * cos(currentAngle) - dirZ * sin(currentAngle);
        render->DrawLine(pRef, pRef + dirNeedle * (radius * 0.8f), glm::vec4(1, 1, 0, 1));
    }
}

//void HingeJoint::Serialize(nlohmann::json& componentObj) const
//{
//    Joint::Serialize(componentObj);
//    componentObj["limitsEnabled"] = limitsEnabled;
//    componentObj["minAngle"] = minAngle;
//    componentObj["maxAngle"] = maxAngle;
//    componentObj["motorEnabled"] = motorEnabled;
//    componentObj["driveVelocity"] = driveVelocity;
//}
//
//void HingeJoint::Deserialize(const nlohmann::json& componentObj)
//{
//    Joint::Deserialize(componentObj);
//
//    if (componentObj.contains("minAngle"))
//        SetMinAngle(componentObj["minAngle"].get<float>());
//    if (componentObj.contains("maxAngle"))
//        SetMaxAngle(componentObj["maxAngle"].get<float>());
//    if (componentObj.contains("limitsEnabled"))
//        EnableLimits(componentObj["limitsEnabled"].get<bool>());
//
//    if (componentObj.contains("driveVelocity"))
//        SetDriveVelocity(componentObj["driveVelocity"].get<float>());
//    if (componentObj.contains("motorEnabled"))
//        EnableMotor(componentObj["motorEnabled"].get<bool>());
//}