#include "Application.h"
#include "Renderer.h"
#include "GameObject.h"

#include "BoxCollider.h"
#include "Transform.h"
#include "Rigidbody.h"

#include "imgui.h"


BoxCollider::BoxCollider(GameObject* owner) : Collider(owner)
{
    name = "Box Collider";
    type = ComponentType::BOX_COLLIDER;
}

physx::PxGeometry* BoxCollider::GetGeometry()
{
    glm::vec3 scale = owner->transform->GetGlobalScale();
    return new physx::PxBoxGeometry((size.x * scale.x) * 0.5f,
        (size.y * scale.y) * 0.5f,
        (size.z * scale.z) * 0.5f);
}

void BoxCollider::Update()
{

}

void BoxCollider::OnEditor()
{
    #ifndef WAVE_GAME
    OnEditorBase();
    ImGui::Separator();

    ImGui::Text("Size");
    glm::vec3 s = size;
    if (ImGui::InputFloat3("##Size", &s.x))
    {
        SetSize(s);
    }
    #endif
}

void BoxCollider::Serialize(nlohmann::json& componentObj) const
{
    SerializeBase(componentObj);

    componentObj["Size"] = { size.x, size.y, size.z };
}

void BoxCollider::Deserialize(const nlohmann::json& componentObj)
{
    DeserializeBase(componentObj);

    if (componentObj.contains("Size")) {
        auto s = componentObj["Size"];
        size = glm::vec3(s[0], s[1], s[2]);
    }
    else {
        size = glm::vec3(1.0f);
    }

    Rigidbody* rb = static_cast<Rigidbody*>(owner->GetComponentInParent(ComponentType::RIGIDBODY));
    if (rb) {
        rb->CreateBody();
    }
}

void BoxCollider::SetSize(glm::vec3 size)
{
    this->size.x = glm::clamp(size.x, 0.001f, INFINITY);
    this->size.y = glm::clamp(size.y, 0.001f, INFINITY);
    this->size.z = glm::clamp(size.z, 0.001f, INFINITY);
    if (attachedRigidbody) attachedRigidbody->UpdateShapesGeometry();
}


void BoxCollider::DebugShape()
{
    physx::PxRigidActor* actor = (attachedRigidbody) ? attachedRigidbody->GetActor() : nullptr;

    glm::vec3 pos;
    glm::quat rot;
    glm::vec4 color;
    glm::vec3 scale = owner->transform->GetGlobalScale();
    glm::vec3 halfSize = (size * scale) * 0.5f;

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

    glm::vec3 v[8] = {
        {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
        {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
    };

    for (int i = 0; i < 8; ++i) {
        v[i] = pos + (rot * (v[i] * halfSize));
    }

    auto* render = Application::GetInstance().renderer.get();

    for (int i = 0; i < 4; ++i) {

        render->DrawLine(v[i], v[(i + 1) % 4], color);
        render->DrawLine(v[i + 4], v[((i + 1) % 4) + 4], color);
        render->DrawLine(v[i], v[i + 4], color);
    }
}

//void BoxCollider::Serialize(nlohmann::json& componentObj) const
//{
//    Collider::Serialize(componentObj);
//    componentObj["size"] = { size.x, size.y, size.z };
//}
//
//void BoxCollider::Deserialize(const nlohmann::json& componentObj)
//{
//    Collider::Deserialize(componentObj);
//    if (componentObj.contains("size")) {
//        const auto& s = componentObj["size"];
//        SetSize(glm::vec3(s[0].get<float>(), s[1].get<float>(), s[2].get<float>()));
//    }
//}