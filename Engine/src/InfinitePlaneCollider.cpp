#include "InfinitePlaneCollider.h"
#include "Application.h"
#include "Renderer.h"
#include "GameObject.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "imgui.h"

InfinitePlaneCollider::InfinitePlaneCollider(GameObject* owner) : Collider(owner) {
    name = "Infinite Plane Collider";
    type = ComponentType::INFINITE_PLANE_COLLIDER;
}

physx::PxGeometry* InfinitePlaneCollider::GetGeometry() 
{
    return new physx::PxPlaneGeometry();
}

void InfinitePlaneCollider::Update()
{

}

void InfinitePlaneCollider::OnEditor() 
{
#ifndef WAVE_GAME
    OnEditorBase();
#endif
}


void InfinitePlaneCollider::Serialize(nlohmann::json& componentObj) const {
    SerializeBase(componentObj);
}

void InfinitePlaneCollider::Deserialize(const nlohmann::json& componentObj) {
    DeserializeBase(componentObj);
    Rigidbody* rb = (Rigidbody*)owner->GetComponentInParent(ComponentType::RIGIDBODY);
    if (rb) rb->CreateBody();
}

void InfinitePlaneCollider::DebugShape() {
   
    physx::PxRigidActor* actor = (attachedRigidbody) ? attachedRigidbody->GetActor() : nullptr;

    auto* render = Application::GetInstance().renderer.get();

    glm::vec3 pos = owner->transform->GetGlobalPosition();
    glm::quat rot = owner->transform->GetGlobalRotationQuat();

    glm::vec3 up = rot * glm::vec3(0, 1, 0);
    glm::vec3 forward = rot * glm::vec3(0, 0, 1);

    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    if (actor && shape)
    {
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    float gridSize = 500.0f;
    int divisions = 10;
    float step = gridSize / divisions;

    for (int i = -divisions; i <= divisions; ++i) {
        float offset = i * step;

        render->DrawLine(pos + (up * (gridSize)) + (forward * offset),
            pos - (up * (gridSize)) + (forward * offset), color);

        render->DrawLine(pos + (forward * (gridSize)) + (up * offset),
            pos - (forward * (gridSize)) + (up * offset), color);
    }

    glm::vec3 normal = rot * glm::vec3(1, 0, 0);
    render->DrawLine(pos, pos + normal * 2.0f, glm::vec4(1, 1, 1, 1));
}

//void InfinitePlaneCollider::Serialize(nlohmann::json& componentObj) const
//{
//    Collider::Serialize(componentObj);
//}
//
//void InfinitePlaneCollider::Deserialize(const nlohmann::json& componentObj)
//{
//    Collider::Deserialize(componentObj);
//}