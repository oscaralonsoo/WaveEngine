#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject* owner, ComponentType type) : owner(owner), type(type), active(true) {
    switch (type) {
    case ComponentType::TRANSFORM: name = "Transform"; break;
    case ComponentType::MESH: name = "Mesh"; break;
    case ComponentType::MATERIAL: name = "Material"; break;
	case ComponentType::CAMERA: name = "Camera"; break;
    case ComponentType::AUDIOSOURCE: name = "AudioSource"; break;
    case ComponentType::LISTENER: name = "Listener"; break;
    case ComponentType::MOVE: name = "Move"; break;
    default: name = "Unknown Component";
    }
}

