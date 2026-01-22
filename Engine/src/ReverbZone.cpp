#include "ReverbZone.h"
#include "Transform.h"
#include "Application.h"
#include "AudioSystem.h"
#include "Log.h"
#include <imgui.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

ReverbZone::ReverbZone(GameObject* owner)
    : Component(owner, ComponentType::REVERBZONE)
{
    name = "Reverb Zone";
    // Register with audio system
    if (Application::GetInstance().audio && Application::GetInstance().audio->audioSystem) {
        Application::GetInstance().audio->audioSystem->RegisterReverbZone(this);
    }
}

ReverbZone::~ReverbZone()
{
    if (Application::GetInstance().audio && Application::GetInstance().audio->audioSystem) {
        Application::GetInstance().audio->audioSystem->UnregisterReverbZone(this);
    }
}

bool ReverbZone::ContainsPoint(const glm::vec3& worldPoint) const
{
    if (!enabled) return false;

    Transform* t = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!t) return false;

    glm::mat4 worldMat = t->GetGlobalMatrix();
    glm::vec3 zonePos = glm::vec3(worldMat[3]);

    if (shape == Shape::SPHERE) {
        float d2 = glm::length(worldPoint - zonePos);
        return d2 <= radius;
    }
    else { // BOX: transform point into local space of the zone (remove rotation + translation)
        glm::mat4 inv = glm::inverse(worldMat);
        glm::vec4 local = inv * glm::vec4(worldPoint, 1.0f);
        glm::vec3 absLocal = glm::abs(glm::vec3(local));
        return (absLocal.x <= extents.x) && (absLocal.y <= extents.y) && (absLocal.z <= extents.z);
    }
}

void ReverbZone::Serialize(nlohmann::json& componentObj) const
{
    componentObj["shape"] = static_cast<int>(shape);
    componentObj["radius"] = radius;
    componentObj["extents"] = { extents.x, extents.y, extents.z };
    componentObj["auxBusName"] = auxBusName;
    componentObj["wetLevel"] = wetLevel;
    componentObj["priority"] = priority;
    componentObj["enabled"] = enabled;
}

void ReverbZone::Deserialize(const nlohmann::json& componentObj)
{
    if (componentObj.contains("shape")) shape = static_cast<Shape>(componentObj["shape"].get<int>());
    if (componentObj.contains("radius")) radius = componentObj["radius"].get<float>();
    if (componentObj.contains("extents")) {
        auto e = componentObj["extents"];
        extents = glm::vec3(e[0].get<float>(), e[1].get<float>(), e[2].get<float>());
    }
    if (componentObj.contains("auxBusName")) auxBusName = componentObj["auxBusName"].get<std::string>();
    if (componentObj.contains("wetLevel")) wetLevel = componentObj["wetLevel"].get<float>();
    if (componentObj.contains("priority")) priority = componentObj["priority"].get<int>();
    if (componentObj.contains("enabled")) enabled = componentObj["enabled"].get<bool>();
}

void ReverbZone::OnEditor()
{
    const char* shapeNames[] = { "Sphere", "Box" };
    int shapeIndex = static_cast<int>(shape);

    if (ImGui::Combo("Shape", &shapeIndex, shapeNames, IM_ARRAYSIZE(shapeNames))) {
        shape = static_cast<Shape>(shapeIndex);
    }

    if (shape == Shape::SPHERE) {
        ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 10000.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Sphere radius in world units");
    }
    else {
        ImGui::DragFloat3("Half Extents", &extents[0], 0.1f, 0.0f, 10000.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Half extents (local space) for box shape");
    }

    char buf[256];
    strncpy(buf, auxBusName.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    if (ImGui::InputText("Aux Bus Name", buf, sizeof(buf))) {
        auxBusName = std::string(buf);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Name of the Wwise Aux Bus to send to (case-sensitive)");

    ImGui::SliderFloat("Wet Level", &wetLevel, 0.0f, 1.0f);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Amount of aux send applied to listener while inside (0..1)");

    ImGui::DragInt("Priority", &priority, 1, -10000, 10000);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Higher priority wins when multiple zones overlap");

    ImGui::Checkbox("Enabled", &enabled);
}