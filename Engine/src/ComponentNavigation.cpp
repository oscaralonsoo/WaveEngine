#pragma once

#include "NavMeshManager.h"
#include "ComponentNavigation.h"
#include "Application.h"
#include "imgui.h"
#include "GameObject.h"

ComponentNavigation::ComponentNavigation(GameObject* owner)
    : Component(owner, ComponentType::NAVIGATION)
{
}

void ComponentNavigation::OnEditor()
{
    if (ImGui::CollapsingHeader("Navigation & AI", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Navigation Static", &isStatic);

        ImGui::Spacing();

        if (ImGui::Button("Bake World NavMesh", ImVec2(-1, 30)))
        {
            if (Application::GetInstance().navMesh) {
                Application::GetInstance().navMesh->Bake(this->owner);
            }
        }
    }
}