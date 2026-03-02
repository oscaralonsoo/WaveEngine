#include "D6Joint.h"
#include "Rigidbody.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "Renderer.h"
#include "imgui.h"

D6Joint::D6Joint(GameObject* owner) : Joint(owner) {
    name = "D6 Joint";
    type = ComponentType::D6_JOINT;
    for (int i = 0; i < 3; ++i) motions[i] = physx::PxD6Motion::eLOCKED;
    for (int i = 3; i < 6; ++i) motions[i] = physx::PxD6Motion::eLOCKED;
    
    RefreshJoint();
}

D6Joint::~D6Joint() {}

void D6Joint::CreateJoint() {
    auto* physics = Application::GetInstance().physics->GetPhysics();
    if (!physics || !bodyA || !bodyA->GetActor()) return;


    bool isADynamic = (bodyA->GetBodyType() == Rigidbody::Type::DYNAMIC);

    bool isBDynamic = (bodyB != nullptr) && (bodyB->GetBodyType() == Rigidbody::Type::DYNAMIC);

    if (!isADynamic && !isBDynamic) {
        //LOG(LogType::LOG_WARNING, "D6 Joint ignored: At least one body must be DYNAMIC.");
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

    pxJoint = physx::PxD6JointCreate(*physics, actorA, localA, actorB, localB);

    if (pxJoint == nullptr) {
        //LOG(LogType::LOG_ERROR, "Joint Error: PhysX failed to create PxD6Joint");
        return;
    }

    pxJoint->setBreakForce(breakForce, breakTorque);
    auto* d6 = static_cast<physx::PxD6Joint*>(pxJoint);

    for (int i = 0; i < 6; ++i) {
        d6->setMotion((physx::PxD6Axis::Enum)i, motions[i]);
    }

    physx::PxJointLinearLimit distanceLimit(linearLimit, physx::PxSpring(0.0f, 0.0f));
    d6->setDistanceLimit(distanceLimit);

    d6->setTwistLimit(physx::PxJointAngularLimitPair(glm::radians(twistMin), glm::radians(twistMax)));
    d6->setSwingLimit(physx::PxJointLimitCone(glm::radians(swingY), glm::radians(swingZ)));
}

void D6Joint::SetMotion(physx::PxD6Axis::Enum axis, physx::PxD6Motion::Enum motion) {
    motions[axis] = motion;
    auto* d6 = static_cast<physx::PxD6Joint*>(pxJoint);
    if (d6) {
        d6->setMotion(axis, motion);
        if (bodyA) bodyA->WakeUp();
    }
}

void D6Joint::SetLinearLimit(float extent) {
    linearLimit = glm::max(0.01f, extent);
    auto* d6 = static_cast<physx::PxD6Joint*>(pxJoint);

    if (d6) {
        
        physx::PxJointLinearLimit limit(linearLimit, physx::PxSpring(0.0f, 0.0f));

        d6->setDistanceLimit(limit);

        if (bodyA) bodyA->WakeUp();
        if (bodyB) bodyB->WakeUp();
    }
}

void D6Joint::SetTwistLimit(float minAngle, float maxAngle) 
{
    float clampedMin = glm::clamp(minAngle, -179.9f, 179.9f);
    float clampedMax = glm::clamp(maxAngle, -179.9f, 179.9f);

    if (clampedMin >= clampedMax) 
    {
        clampedMin = clampedMax - 0.01f;
    }

    twistMin = clampedMin;
    twistMax = clampedMax;

    auto* d6 = static_cast<physx::PxD6Joint*>(pxJoint);
    if (d6) 
    {
        physx::PxJointAngularLimitPair limit(glm::radians(twistMin), glm::radians(twistMax));

        d6->setTwistLimit(limit);

        if (bodyA) bodyA->WakeUp();
    }
}

void D6Joint::SetSwingLimit(float yAngle, float zAngle) {

    swingY = glm::clamp(yAngle, 0.01f, 179.9f);
    swingZ = glm::clamp(zAngle, 0.01f, 179.9f);

    auto* d6 = static_cast<physx::PxD6Joint*>(pxJoint);
    if (d6) {

        physx::PxJointLimitCone limit(glm::radians(swingY), glm::radians(swingZ));

        d6->setSwingLimit(limit);
        if (bodyA) bodyA->WakeUp();
    }
}

void D6Joint::Serialize(nlohmann::json& componentObj) const
{
    SerializeBase(componentObj);

    nlohmann::json motionsArray = nlohmann::json::array();
    for (int i = 0; i < 6; ++i)
    {
        motionsArray.push_back(static_cast<int>(motions[i]));
    }
    componentObj["Motions"] = motionsArray;

    componentObj["LinearLimit"] = linearLimit;
    componentObj["TwistLimit"] = { twistMin, twistMax };
    componentObj["SwingLimit"] = { swingY, swingZ };
}

void D6Joint::Deserialize(const nlohmann::json& componentObj)
{
    DeserializeBase(componentObj);

    if (componentObj.contains("Motions") && componentObj["Motions"].is_array())
    {
        for (int i = 0; i < 6 && i < componentObj["Motions"].size(); ++i)
        {
            motions[i] = static_cast<physx::PxD6Motion::Enum>(componentObj["Motions"][i].get<int>());
        }
    }

    linearLimit = componentObj.value("LinearLimit", 1.0f);

    if (componentObj.contains("TwistLimit")) {
        auto t = componentObj["TwistLimit"];
        twistMin = t[0];
        twistMax = t[1];
    }

    if (componentObj.contains("SwingLimit")) {
        auto s = componentObj["SwingLimit"];
        swingY = s[0];
        swingZ = s[1];
    }

    RefreshJoint();
}

void D6Joint::OnEditor() {
#ifndef WAVE_GAME
    OnEditorBase();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth;

    const char* axisNames[] = { "X", "Y", "Z", "Twist (Rot X)", "Swing Y", "Swing Z" };
    const char* motionNames[] = { "Locked", "Limited", "Free" };

    if (ImGui::TreeNodeEx("Motions", flags)) {
        for (int i = 0; i < 6; ++i) {
            int current = (int)motions[i];
            if (ImGui::Combo(axisNames[i], &current, motionNames, 3)) {
                SetMotion((physx::PxD6Axis::Enum)i, (physx::PxD6Motion::Enum)current);
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Limits", flags)) {
        if (ImGui::DragFloat("Linear Extent", &linearLimit, 0.1f, 0.0f, 100.0f)) SetLinearLimit(linearLimit);
        if (ImGui::DragFloat2("Twist Min/Max", &twistMin, 1.0f, -180.0f, 180.0f)) SetTwistLimit(twistMin, twistMax);
        if (ImGui::DragFloat2("Swing Y/Z", &swingY, 1.0f, 0.0f, 180.0f)) SetSwingLimit(swingY, swingZ);
        ImGui::TreePop();
    }
#endif
}

void D6Joint::DrawDebug() {
    
    if (!bodyA || !pxJoint) return;

    physx::PxTransform poseRef = (bodyB) ? bodyB->GetActor()->getGlobalPose() : physx::PxTransform(physx::PxIdentity);
    physx::PxTransform worldRef = poseRef.transform(pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1));

    glm::vec3 pRef(worldRef.p.x, worldRef.p.y, worldRef.p.z);
    glm::vec3 dirX(worldRef.q.getBasisVector0().x, worldRef.q.getBasisVector0().y, worldRef.q.getBasisVector0().z);
    glm::vec3 dirY(worldRef.q.getBasisVector1().x, worldRef.q.getBasisVector1().y, worldRef.q.getBasisVector1().z);
    glm::vec3 dirZ(worldRef.q.getBasisVector2().x, worldRef.q.getBasisVector2().y, worldRef.q.getBasisVector2().z);

    auto* render = Application::GetInstance().renderer.get();

    render->DrawLine(pRef, pRef + dirX * 0.5f, glm::vec4(1, 0, 0, 1));
    render->DrawLine(pRef, pRef + dirY * 0.5f, glm::vec4(0, 1, 0, 1));
    render->DrawLine(pRef, pRef + dirZ * 0.5f, glm::vec4(0, 0, 1, 1));

    if (motions[0] == physx::PxD6Motion::eLIMITED || motions[1] == physx::PxD6Motion::eLIMITED || motions[2] == physx::PxD6Motion::eLIMITED) {
        render->DrawSphere(pRef, linearLimit, glm::vec4(1, 1, 1, 0.2f));
    }

    physx::PxTransform poseA = bodyA->GetActor()->getGlobalPose();
    physx::PxTransform worldA = poseA.transform(pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0));
    render->DrawLine(pRef, glm::vec3(worldA.p.x, worldA.p.y, worldA.p.z), glm::vec4(1, 1, 1, 1));
}

//void D6Joint::Serialize(nlohmann::json& componentObj) const
//{
//    Joint::Serialize(componentObj);
//
//    nlohmann::json motionsArr = nlohmann::json::array();
//    for (int i = 0; i < 6; ++i)
//        motionsArr.push_back((int)motions[i]);
//    componentObj["motions"] = motionsArr;
//
//    componentObj["linearLimit"] = linearLimit;
//    componentObj["twistMin"] = twistMin;
//    componentObj["twistMax"] = twistMax;
//    componentObj["swingY"] = swingY;
//    componentObj["swingZ"] = swingZ;
//}
//
//void D6Joint::Deserialize(const nlohmann::json& componentObj)
//{
//    Joint::Deserialize(componentObj);
//
//    if (componentObj.contains("motions")) {
//        const auto& arr = componentObj["motions"];
//        for (int i = 0; i < 6 && i < (int)arr.size(); ++i)
//            motions[i] = (physx::PxD6Motion::Enum)arr[i].get<int>();
//    }
//
//    if (componentObj.contains("linearLimit"))
//        SetLinearLimit(componentObj["linearLimit"].get<float>());
//
//    float tMin = twistMin, tMax = twistMax;
//    if (componentObj.contains("twistMin")) tMin = componentObj["twistMin"].get<float>();
//    if (componentObj.contains("twistMax")) tMax = componentObj["twistMax"].get<float>();
//    SetTwistLimit(tMin, tMax);
//
//    float sY = swingY, sZ = swingZ;
//    if (componentObj.contains("swingY")) sY = componentObj["swingY"].get<float>();
//    if (componentObj.contains("swingZ")) sZ = componentObj["swingZ"].get<float>();
//    SetSwingLimit(sY, sZ);
//}