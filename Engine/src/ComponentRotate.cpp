#include "ComponentRotate.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "Log.h"
#include <imgui.h>

ComponentRotate::ComponentRotate(GameObject* owner)
    : Component(owner, ComponentType::ROTATE),
    rotationSpeed(0.0f, 45.0f, 0.0f)
{
    name = "Rotate";
}

void ComponentRotate::Update()
{
    if (!active) return;

    float dt = Application::GetInstance().time->GetDeltaTime();

    if (dt == 0.0f) return;

    Transform* transform = static_cast<Transform*>(
        owner->GetComponent(ComponentType::TRANSFORM));

    if (!transform) return;

    // Apply rotation based on deltaTime
    glm::vec3 currentRotation = transform->GetRotation();
    glm::vec3 deltaRotation = rotationSpeed * dt;
    transform->SetRotation(currentRotation + deltaRotation);
}

void ComponentRotate::OnEditor()
{
    ImGui::DragFloat3("Rotation Speed (deg/s)", &rotationSpeed.x, 1.0f, -360.0f, 360.0f);
}