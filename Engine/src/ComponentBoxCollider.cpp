#include "ComponentBoxCollider.h"
#include <btBulletDynamicsCommon.h>
#include <imgui.h>

ComponentBoxCollider::ComponentBoxCollider(GameObject* owner) 
    : ComponentCollider(owner, ComponentType::COLLIDER_BOX) 
{
    shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
}


void ComponentBoxCollider::SetSize(const glm::vec3& newSize) {
    size = newSize;
    if (shape) delete shape;
    
    // Bullet usa "half extents", por eso multiplicamos por 0.5
    shape = new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
}

void ComponentBoxCollider::OnEditor() {
    if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::DragFloat3("Size", &size[0], 0.1f)) {
            SetSize(size);
        }
    }
}