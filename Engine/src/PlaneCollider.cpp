#include "Application.h"
#include "Renderer.h"
#include "GameObject.h"

#include "PlaneCollider.h"
#include "Transform.h"
#include "Rigidbody.h"

#include "imgui.h"


PlaneCollider::PlaneCollider(GameObject* owner) : Collider(owner)
{
    name = "Plane Collider"; 
    type = ComponentType::PLANE_COLLIDER;
}

physx::PxGeometry* PlaneCollider::GetGeometry()
{
    glm::vec3 scale = owner->transform->GetGlobalScale();
    return new physx::PxBoxGeometry((size.x * scale.x) * 0.5f,
        0.001f * 0.5f,
        (size.y * scale.z) * 0.5f);
}

void PlaneCollider::Update()
{

}

void PlaneCollider::OnEditor()
{
#ifndef WAVE_GAME
    OnEditorBase();
    ImGui::Separator();

    ImGui::Text("Size");
    glm::vec2 s = size;
    if (ImGui::InputFloat2("##Size", &s.x))
    {
        SetSize(s);
    }
#endif
}

void PlaneCollider::Serialize(nlohmann::json& componentObj) const
{
    SerializeBase(componentObj);

    componentObj["SizeX"] = size.x;
    componentObj["SizeY"] = size.y;
}

void PlaneCollider::Deserialize(const nlohmann::json& componentObj)
{
    DeserializeBase(componentObj);

    size.x = componentObj.value("SizeX", 1.0f);
    size.y = componentObj.value("SizeY", 1.0f);

    Rigidbody* rb = static_cast<Rigidbody*>(owner->GetComponentInParent(ComponentType::RIGIDBODY));
    if (rb) {
        rb->CreateBody();
    }
}

void PlaneCollider::SetSize(glm::vec2 size)
{
    this->size.x = glm::clamp(size.x, 0.001f, INFINITY);
    this->size.y = glm::clamp(size.y, 0.001f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapesGeometry();
}


void PlaneCollider::DebugShape()
{
    physx::PxRigidActor* actor = (attachedRigidbody) ? attachedRigidbody->GetActor() : nullptr;

    glm::vec3 pos;
    glm::quat rot;
    glm::vec4 color;

    glm::vec3 globalScale = owner->transform->GetGlobalScale();

    glm::vec3 actualHalfSize(
        (size.x * globalScale.x) * 0.5f,
        0.001f * 0.5f,
        (size.y * globalScale.z) * 0.5f
    );

    if (actor && shape)
    {
        physx::PxTransform worldPose = actor->getGlobalPose() * shape->getLocalPose();

        pos = glm::vec3(worldPose.p.x, worldPose.p.y, worldPose.p.z);
        rot = glm::quat(worldPose.q.w, worldPose.q.x, worldPose.q.y, worldPose.q.z);
        color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        glm::quat globalRot = owner->transform->GetGlobalRotationQuat();

        pos = owner->transform->GetGlobalPosition() + (globalRot * (center * globalScale));
        rot = globalRot;
        color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    glm::vec3 v[8] = {
        {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
        {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
    };

    for (int i = 0; i < 8; ++i) {
        v[i] = pos + (rot * (v[i] * actualHalfSize));
    }

    auto* render = Application::GetInstance().renderer.get();
    for (int i = 0; i < 4; ++i) {
        render->DrawLine(v[i], v[(i + 1) % 4], color);
        render->DrawLine(v[i + 4], v[((i + 4 + 1) % 4) + 4], color);
        render->DrawLine(v[i], v[i + 4], color);
    }
}

//void PlaneCollider::Serialize(nlohmann::json& componentObj) const
//{
//    Collider::Serialize(componentObj);
//    componentObj["size"] = { size.x, size.y };
//}
//
//void PlaneCollider::Deserialize(const nlohmann::json& componentObj)
//{
//    Collider::Deserialize(componentObj);
//    if (componentObj.contains("size")) {
//        const auto& s = componentObj["size"];
//        SetSize(glm::vec2(s[0].get<float>(), s[1].get<float>()));
//    }
//}