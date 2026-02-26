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

        //Select Type
        const char* items[] = { "Surface", "Agent", "Obstacle" };
     
        int currentType = static_cast<int>(type);
        if (ImGui::Combo("Nav Type", &currentType, items, IM_ARRAYSIZE(items)))
        {
            type = static_cast<NavType>(currentType);
        }

        ImGui::Spacing();

        if (type == NavType::SURFACE) {
            ImGui::Text("Surface Settings");
            ImGui::SliderFloat("Max Slope Angle", &maxSlopeAngle, 0.0f, 90.0f);
        }


        ImGui::Spacing();

        if (ImGui::Button("Bake NavMesh", ImVec2(-1, 30)))
        {
            Application::GetInstance().navMesh->Bake(this->owner);

         
        }

        ImGui::Spacing();

        if (ImGui::Button("Clear NavMesh", ImVec2(-1, 30)))
        {

            if (Application::GetInstance().navMesh)
            {
                Application::GetInstance().navMesh->RemoveNavMesh(owner);
            }

          
        }

    }
}