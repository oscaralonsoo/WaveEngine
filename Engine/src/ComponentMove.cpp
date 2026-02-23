#include "ComponentMove.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "Time.h"
#include <imgui.h>

ComponentMove::ComponentMove(GameObject* owner)
    : Component(owner, ComponentType::MOVE) {
} 

void ComponentMove::Update() {
    //only apply in playing state
    if (Application::GetInstance().GetPlayState() != Application::PlayState::PLAYING)
        return;

    Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans) {
        float dt = Application::GetInstance().time->GetDeltaTime();
        glm::vec3 pos = trans->GetPosition();

        float step = speed * dt;
        if (movingForward) {
            pos += direction * step;
            timer += step;
        }
        else {
            pos -= direction * step;
            timer += step;
        }

        if (timer >= distance) {
            movingForward = !movingForward;
            timer = 0.0f;
        }

        trans->SetPosition(pos);
    }
}

void ComponentMove::OnEditor() {
    //if (ImGui::CollapsingHeader("Auto Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
    //    ImGui::DragFloat("Speed", &speed, 0.0f);
    //    ImGui::DragFloat("Max Distance", &distance, 0.1f);
    //    ImGui::DragFloat3("Direction", &direction.x, 0.1f);
    //}
}

void ComponentMove::Serialize(nlohmann::json& componentObj) const {
    //componentObj["speed"] = speed;
    //componentObj["distance"] = distance;
    //componentObj["dirX"] = direction.x;
    //componentObj["dirY"] = direction.y;
    //componentObj["dirZ"] = direction.z;
}

void ComponentMove::Deserialize(const nlohmann::json& componentObj) {
    //speed = componentObj.value("speed", 2.0f);
    //distance = componentObj.value("distance", 5.0f);
    //direction.x = componentObj.value("dirX", 0.0f);
    //direction.y = componentObj.value("dirY", 0.0f);
    //direction.z = componentObj.value("dirZ", 1.0f);
}