#include "Application.h"
#include "Renderer.h"
#include "GameObject.h"

#include "SphereCollider.h"
#include "Transform.h"
#include "Rigidbody.h"
#include "imgui.h"

SphereCollider::SphereCollider(GameObject* owner) : Collider(owner)
{
    name = "Sphere Collider";
    type = ComponentType::SPHERE_COLLIDER;
}

physx::PxGeometry* SphereCollider::GetGeometry()
{
    glm::vec3 scale = owner->transform->GetGlobalScale();

    float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));

    return new physx::PxSphereGeometry(radius * maxScale);
}

void SphereCollider::Update()
{
    DebugShape();
}

void SphereCollider::OnEditor()
{
    OnEditorBase();

    ImGui::Separator();

    ImGui::Text("Radius");
    float r = radius;
    if (ImGui::InputFloat("##Radius", &r))
    {
        SetRadius(r);
    }
}

//void SphereCollider::Save(Config& config)
//{
//    SaveBase(config);
//    config.SetFloat("Radius", radius);
//}
//
//void SphereCollider::Load(Config& config)
//{
//    LoadBase(config);
//    SetRadius(config.GetFloat("Radius", 1.0f));
//    
//}

void SphereCollider::SetRadius(float radius)
{
    this->radius = glm::clamp(radius, 0.0001f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapesGeometry();
}

void SphereCollider::DebugShape()
{
    physx::PxRigidActor* actor = (attachedRigidbody) ? attachedRigidbody->GetActor() : nullptr;

    glm::vec3 pos;
    glm::quat rot;
    glm::vec4 color;
    glm::vec3 scale = owner->transform->GetGlobalScale();

    if (actor && shape)
    {
        physx::PxTransform worldPose = actor->getGlobalPose();
        worldPose = worldPose * shape->getLocalPose();

        pos = glm::vec3(worldPose.p.x, worldPose.p.y, worldPose.p.z);
        rot = glm::quat(worldPose.q.w, worldPose.q.x, worldPose.q.y, worldPose.q.z);
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        glm::quat globalRot = owner->transform->GetGlobalRotationQuat();

        pos = owner->transform->GetGlobalPosition() + (globalRot * (center * scale));
        rot = globalRot;
        color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Rojo
    }

    float maxScale = glm::max(scale.x, glm::max(scale.y, scale.z));
    float finalRadius = radius * maxScale;

    auto* render = Application::GetInstance().renderer.get();
    int segments = 16;

    render->DrawCircle(pos, rot, finalRadius, segments, color, glm::vec3(1, 0, 0), glm::vec3(0, 0, 1));
    render->DrawCircle(pos, rot, finalRadius, segments, color, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
    render->DrawCircle(pos, rot, finalRadius, segments, color, glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
}