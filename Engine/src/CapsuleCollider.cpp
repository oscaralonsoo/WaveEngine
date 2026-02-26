#include "Application.h"
#include "Renderer.h"
#include "GameObject.h"

#include "CapsuleCollider.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "imgui.h"
#include "glm/glm.hpp"

CapsuleCollider::CapsuleCollider(GameObject* owner) : Collider(owner)
{
    name = "Capsule Collider";
    type = ComponentType::CAPSULE_COLLIDER;
}

physx::PxGeometry* CapsuleCollider::GetGeometry() {
    
    glm::vec3 scale = owner->transform->GetGlobalScale();
   
    float s = glm::max(scale.x, scale.z);
    
    return new physx::PxCapsuleGeometry(radius * s, (height * scale.y) * 0.5f);
}

void CapsuleCollider::Update()
{
    DebugShape();
}


void CapsuleCollider::OnEditor()
{
    #ifndef WAVE_GAME
    OnEditorBase();

    ImGui::Separator();

    ImGui::Text("Radius");
    float r = radius;
    if (ImGui::InputFloat("##Radius", &r))
    {
        SetRadius(r);
    }

    ImGui::Text("Height");
    float h = height;
    if (ImGui::InputFloat("##Height", &h))
    {
        SetHeight(h);
    }
#endif
}

void CapsuleCollider::Serialize(nlohmann::json& componentObj) const
{
    SerializeBase(componentObj);

    componentObj["Radius"] = radius;
    componentObj["Height"] = height;
}

void CapsuleCollider::Deserialize(const nlohmann::json& componentObj)
{
    DeserializeBase(componentObj);

    SetRadius(componentObj.value("Radius", 0.5f));
    SetHeight(componentObj.value("Height", 1.0f));

    Rigidbody* rb = static_cast<Rigidbody*>(owner->GetComponentInParent(ComponentType::RIGIDBODY));
    if (rb) {
        rb->CreateBody();
    }
}

void CapsuleCollider::SetRadius(float radius)
{
    this->radius = glm::clamp(radius, 0.0001f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapesGeometry();
}

void CapsuleCollider::SetHeight(float height)
{
    this->height = glm::clamp(height, 0.0001f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapesGeometry();
}

void CapsuleCollider::DebugShape()
{
    physx::PxRigidActor* actor = (attachedRigidbody) ? attachedRigidbody->GetActor() : nullptr;

    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale = owner->transform->GetGlobalScale();
    glm::vec4 color;

    glm::vec3 localUp, localRight, localForward;

    if (actor && shape)
    {
        physx::PxTransform worldPose = actor->getGlobalPose();
        if (shape) worldPose = worldPose * shape->getLocalPose();

        pos = glm::vec3(worldPose.p.x, worldPose.p.y, worldPose.p.z);
        rot = glm::quat(worldPose.q.w, worldPose.q.x, worldPose.q.y, worldPose.q.z);
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);

        localUp = glm::vec3(1, 0, 0);
        localRight = glm::vec3(0, 1, 0);
        localForward = glm::vec3(0, 0, 1);
    }
    else
    {
        glm::quat globalRot = owner->transform->GetGlobalRotationQuat();
        pos = owner->transform->GetGlobalPosition() + (globalRot * (center * scale));
        rot = globalRot;
        color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

        localUp = glm::vec3(0, 1, 0);
        localRight = glm::vec3(1, 0, 0);
        localForward = glm::vec3(0, 0, 1);
    }

    float finalRadius = radius * glm::max(scale.x, scale.z);
    float finalHalfHeight = (height * 0.5f) * scale.y;

    glm::vec3 worldUp = rot * localUp;
    glm::vec3 worldRight = rot * localRight;
    glm::vec3 worldForward = rot * localForward;

    auto* render = Application::GetInstance().renderer.get();

    glm::vec3 topCenter = pos + (worldUp * finalHalfHeight);
    glm::vec3 bottomCenter = pos - (worldUp * finalHalfHeight);

    render->DrawCircle(topCenter, rot, finalRadius, 16, color, localRight, localForward);
    render->DrawCircle(bottomCenter, rot, finalRadius, 16, color, localRight, localForward);

    render->DrawLine(topCenter + worldRight * finalRadius, bottomCenter + worldRight * finalRadius, color);
    render->DrawLine(topCenter - worldRight * finalRadius, bottomCenter - worldRight * finalRadius, color);
    render->DrawLine(topCenter + worldForward * finalRadius, bottomCenter + worldForward * finalRadius, color);
    render->DrawLine(topCenter - worldForward * finalRadius, bottomCenter - worldForward * finalRadius, color);

    render->DrawArc(topCenter, rot, finalRadius, 8, color, localRight, localUp);
    render->DrawArc(topCenter, rot, finalRadius, 8, color, localForward, localUp);
    render->DrawArc(bottomCenter, rot, finalRadius, 8, color, localRight, -localUp);
    render->DrawArc(bottomCenter, rot, finalRadius, 8, color, localForward, -localUp);
}

//void CapsuleCollider::Serialize(nlohmann::json& componentObj) const
//{
//    Collider::Serialize(componentObj);
//    componentObj["radius"] = radius;
//    componentObj["height"] = height;
//}
//
//void CapsuleCollider::Deserialize(const nlohmann::json& componentObj)
//{
//    Collider::Deserialize(componentObj);
//    if (componentObj.contains("radius"))
//        SetRadius(componentObj["radius"].get<float>());
//    if (componentObj.contains("height"))
//        SetHeight(componentObj["height"].get<float>());
//}
