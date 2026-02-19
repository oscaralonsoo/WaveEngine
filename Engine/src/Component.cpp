#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject* owner, ComponentType type) : owner(owner), type(type), active(true) {
    switch (type) {
    case ComponentType::TRANSFORM: name = "Transform"; break;
    case ComponentType::MESH: name = "Mesh"; break;
    case ComponentType::MATERIAL: name = "Material"; break;
    case ComponentType::CAMERA: name = "Camera"; break;
    case ComponentType::SCRIPT: name = "Script"; break;  
    default: name = "Unknown Component";
    }
}
