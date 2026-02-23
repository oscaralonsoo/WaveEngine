#include "Component.h"
#include "GameObject.h"
#include <imgui.h>  
#include <ImGuizmo.h>

Component::Component(GameObject* owner, ComponentType type) : owner(owner), type(type), active(true) {
    switch (type) {
    case ComponentType::TRANSFORM: name = "Transform"; break;
    case ComponentType::MESH: name = "Mesh"; break;
    case ComponentType::MATERIAL: name = "Material"; break;
    case ComponentType::CAMERA: name = "Camera"; break;
    case ComponentType::SCRIPT: name = "Script"; break;  
    case ComponentType::AUDIOSOURCE: name = "AudioSource"; break;
    case ComponentType::LISTENER: name = "Listener"; break;
    case ComponentType::REVERBZONE: name = "ReverbZone"; break;
    default: name = "Unknown Component";
    }
}

void Component::DrawRemoveComponentPopup() {

    //std::string popupID = "RemovePopup##" + name;
    std::string popupID = name + "RemovePopup##" + std::to_string((uintptr_t)this); 

    if (ImGui::BeginPopupContextItem(popupID.c_str()))
    {
        if (ImGui::MenuItem("Remove Component"))
        {
            markedForRemoval = true;
        }
        ImGui::EndPopup();
    }
}

