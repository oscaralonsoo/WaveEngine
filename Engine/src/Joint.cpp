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

//void Joint::SaveBase(Config& config)
//{
//    config.SetUInt("BodyBUID", bodyB ? bodyB->owner->UUID : 0);
//    config.SetVector3("LocalPositionA", localPosA);
//    config.SetVector3("LocalPositionB", localPosB);
//    config.SetVector3("LocalRotationA", QuatToEuler(localRotA));
//    config.SetVector3("LocalRotationB", QuatToEuler(localRotB));
//    config.SetFloat("BreakForce", breakForce);
//    config.SetFloat("BreakTorque", breakTorque);
//}
//
//void Joint::LoadBase(Config& config)
//{
//    bUID = config.GetUInt("BodyBUID");
//    SetAnchorPosition(JointBody::Self, config.GetVector3("LocalPositionA"));
//    SetAnchorPosition(JointBody::Target, config.GetVector3("LocalPositionB"));
//    SetAnchorRotation(JointBody::Self, EulerToQuat(config.GetVector3("LocalRotationA")));
//    SetAnchorRotation(JointBody::Target, EulerToQuat(config.GetVector3("LocalRotationB")));
//    SetBreakForce(config.GetFloat("BreakForce", INFINITY_PHYSIC));
//    SetBreakTorque(config.GetFloat("BreakTorque", INFINITY_PHYSIC));
//}
//
//void Joint::ResolveReferences()
//{
//    if (bUID != 0) {
//        GameObject* target = Engine::GetInstance().moduleScene->GetObjectByUUID(bUID);
//        if (target) {
//            bodyB = (Rigidbody*)target->GetComponent(ComponentType::Rigidbody);
//        }
//    }
//
//    RefreshJoint();
//}

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

void Joint::OnEditorBase()
{
#ifndef WAVE_GAME
    ImGui::Text("Connections:");
    ImGui::Separator();

    if (ImGui::BeginTable("JointConnections", 2, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Labels", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableSetupColumn("Values", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Body A:");
        ImGui::TableNextColumn();
        if (bodyA)
            ImGui::Text(bodyA->owner->name.c_str());
        else
            ImGui::TextDisabled("None");

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Body B:");
        ImGui::TableNextColumn();
        if (bodyB)
            ImGui::Button(bodyB->owner->name.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 20));
        else
            ImGui::Button("None (World)", ImVec2(ImGui::GetContentRegionAvail().x, 20));
        /*if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(GAMEOBJECTS_DRAG))
            {
                uint32_t* uidsIdx = (uint32_t*)payload->Data;
                int objectCount = payload->DataSize / sizeof(uint32_t);

                for (int i = 0; i < objectCount; i++)
                {
                    uint32_t draggedUID = uidsIdx[i];
                    GameObject* draggedGO = Engine::GetInstance().moduleScene->GetObjectByUUID(draggedUID);

                    if (draggedGO != nullptr)
                    {
                        SetTarget(draggedGO);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }*/
        if (ImGui::Button("Clear"))
        {
            SetTarget(nullptr);
        }

        ImGui::EndTable();
    }
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool isNodeOpen = ImGui::TreeNodeEx("Body A Settings", flags);

    if (isNodeOpen)
    {
        ImGui::Text("Offset Position");

        if (ImGui::InputFloat3("##PositionSelf", &localPosA.x)) {
            SetAnchorPosition(JointBody::Self, localPosA);
        }

        ImGui::Text("Offset Rotation");
        glm::vec3 eulerA = QuatToEuler(localRotA);
        if (ImGui::InputFloat3("##RotationSelf", &eulerA.x)) {
            SetAnchorRotation(JointBody::Self, EulerToQuat(eulerA));
        }
        ImGui::TreePop();
    }
    
    isNodeOpen = ImGui::TreeNodeEx("Body B Settings", flags);

    if (isNodeOpen)
    {
        ImGui::Text("Offset Position");

        if (ImGui::InputFloat3("##PositionTarget", &localPosB.x)) {
            SetAnchorPosition(JointBody::Target, localPosB);
        }

        ImGui::Text("Offset Rotation");
        glm::vec3 eulerB = QuatToEuler(localRotB);
        if (ImGui::InputFloat3("##RotationTarget", &eulerB.x)) {
            SetAnchorRotation(JointBody::Target, EulerToQuat(eulerB));
        }
        ImGui::TreePop();
    }

    isNodeOpen = ImGui::TreeNodeEx("Break Settings", flags);
    
    if (isNodeOpen) {
        const char* forceFormat = (breakForce >= INFINITY_PHYSIC) ? "INFINITY" : "%.3f";
        const char* torqueFormat = (breakTorque >= INFINITY_PHYSIC) ? "INFINITY" : "%.3f";

        ImGui::Text("Break Force");
        if (ImGui::InputFloat("##BreakForce", &breakForce, 0.0f, 0.0f, forceFormat)) {
            SetBreakForce(breakForce);
        }

        ImGui::Text("Break Torque");
        if (ImGui::InputFloat("##BreakTorque", &breakTorque, 0.0f, 0.0f, torqueFormat)) {
            SetBreakTorque(breakTorque);
        }

        if (breakForce < INFINITY_PHYSIC || breakTorque < INFINITY_PHYSIC) {
            if (ImGui::Button("Reset")) {
                SetBreakForce(INFINITY_PHYSIC);
                SetBreakTorque(INFINITY_PHYSIC);
            }
        }

        ImGui::TreePop();
    }
#endif
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