#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentCamera.h"
#include "ComponentRotate.h"
#include "ComponentScript.h"
#include "Rigidbody.h"
#include "BoxCollider.h"
#include "SphereCollider.h"
#include "CapsuleCollider.h"
#include "MeshCollider.h"
#include "ConvexCollider.h"
#include "PlaneCollider.h"
#include "InfinitePlaneCollider.h"
#include "FixedJoint.h"
#include "DistanceJoint.h"
#include "PrismaticJoint.h"
#include "SphericalJoint.h"
#include "D6Joint.h"
#include "HingeJoint.h"
#include "ComponentParticleSystem.h"
#include "ComponentNavigation.h"
#include "ComponentRotate.h"
#include "AudioComponent.h"
#include "AudioSource.h"
#include "AudioListener.h"
#include "ReverbZone.h"
#include <nlohmann/json.hpp>

GameObject::GameObject(const std::string& name) : name(name), active(true), parent(nullptr) {
    CreateComponent(ComponentType::TRANSFORM);
}

GameObject::~GameObject() {
    
    MarkCleaning();

    components.clear();
    componentOwners.clear();

    for (auto* child : children) {
        delete child;
    }
    children.clear();
}

Component* GameObject::CreateComponent(ComponentType type) {
    
    Component* newComponent = nullptr;

    switch (type) {
    case ComponentType::TRANSFORM:
        if (GetComponent(ComponentType::TRANSFORM) != nullptr) {
            return GetComponent(ComponentType::TRANSFORM);
        }
        newComponent = new Transform(this);
        if (newComponent) transform = (Transform*)newComponent;
        break;
    case ComponentType::MESH:
        newComponent = new ComponentMesh(this);
        break;
    case ComponentType::MATERIAL:
        if (GetComponent(ComponentType::MATERIAL) != nullptr) {
            return GetComponent(ComponentType::MATERIAL);
        }
        newComponent = new ComponentMaterial(this);
        break;
    case ComponentType::AUDIOSOURCE:
        newComponent = new AudioSource(this);
        break;
    case ComponentType::LISTENER:
        newComponent = new AudioListener(this);
        break;
    case ComponentType::CAMERA:
        if (GetComponent(ComponentType::CAMERA) != nullptr) {
            return GetComponent(ComponentType::CAMERA);
        }
        newComponent = new ComponentCamera(this);
        break;
    case ComponentType::ROTATE:
        newComponent = new ComponentRotate(this);
        break;
    case ComponentType::REVERBZONE:
        newComponent = new ReverbZone(this);
        break;
    case ComponentType::SCRIPT:
        newComponent = new ComponentScript(this);
        break;
    case ComponentType::PARTICLE:
        newComponent = new ComponentParticleSystem(this);
        break;
    case ComponentType::RIGIDBODY:
        newComponent = new Rigidbody(this);
        break;
    case ComponentType::BOX_COLLIDER:
        newComponent = new BoxCollider(this);
        break;
    case ComponentType::SPHERE_COLLIDER:
        newComponent = new SphereCollider(this);
        break;
    case ComponentType::CAPSULE_COLLIDER:
        newComponent = new CapsuleCollider(this);
        break;
    case ComponentType::CONVEX_COLLIDER:
        newComponent = new ConvexCollider(this);
        break;
    case ComponentType::MESH_COLLIDER:
        newComponent = new MeshCollider(this);
        break;
    case ComponentType::PLANE_COLLIDER:
        newComponent = new PlaneCollider(this);
        break;
    case ComponentType::INFINITE_PLANE_COLLIDER:
        newComponent = new InfinitePlaneCollider(this);
        break;
    case ComponentType::FIXED_JOINT:
        newComponent = new FixedJoint(this);
        break;
    case ComponentType::HINGE_JOINT:
        newComponent = new HingeJoint(this);
        break;
    case ComponentType::SPHERICAL_JOINT:
        newComponent = new SphericalJoint(this);
        break;
    case ComponentType::PRISMATIC_JOINT:
        newComponent = new PrismaticJoint(this);
        break;
    case ComponentType::D6_JOINT:
        newComponent = new D6Joint(this);
        break;
    case ComponentType::DISTANCE_JOINT:
        newComponent = new DistanceJoint(this);
        break;
    case ComponentType::NAVIGATION:
        if (GetComponent(ComponentType::NAVIGATION) != nullptr) {
            return GetComponent(ComponentType::NAVIGATION);
        }
        newComponent = new ComponentNavigation(this);
        break;

    default:
        LOG_DEBUG("ERROR: Unknown component type requested for GameObject '%s'", name.c_str());
        LOG_CONSOLE("Failed to create component");
        return nullptr;
    }

    if (newComponent) {
        componentOwners.push_back(std::unique_ptr<Component>(newComponent));
        components.push_back(newComponent);
    }

    return newComponent;
}

Component* GameObject::GetComponent(ComponentType type) const {
    for (auto* comp : components) {
        if (comp->IsType(type)) {
            return comp;
        }
    }
    return nullptr;
}

std::vector<Component*> GameObject::GetComponentsOfType(ComponentType type) const {
    std::vector<Component*> result;
    for (auto* comp : components) {
        if (comp->IsType(type)) {
            result.push_back(comp);
        }
    }
    return result;
}

Component* GameObject::GetComponentInChildren(ComponentType type)
{
    Component* component = GetComponent(type);

    if (component) return component;

    for (GameObject* child : children)
    {
        if (child)
        {
            component = child->GetComponentInChildren(type);
            if (component) return component;
        }
    }

    return nullptr;
}

void GameObject::GetComponentsInChildren(ComponentType type, std::vector<Component*>& outList)
{
    for (Component* component : components)
    {
        if (component && component->IsType(type))
        {
            outList.push_back(component);
        }
    }

    for (GameObject* child : children)
    {
        if (child) child->GetComponentsInChildren(type, outList);
    }
}

Component* GameObject::GetComponentInParent(ComponentType type)
{
    Component* component = GetComponent(type);
    if (component) return component;

    if (parent != nullptr)
    {
        return parent->GetComponentInParent(type);
    }

    return nullptr;
}

void GameObject::GetComponentsInParent(ComponentType type, std::vector<Component*>& outList)
{
    std::vector<Component*> localComponents = GetComponentsOfType(type);

    if (!localComponents.empty())
    {
        outList.insert(outList.end(), localComponents.begin(), localComponents.end());
    }

    if (parent != nullptr)
    {
        parent->GetComponentsInParent(type, outList);
    }
}

void GameObject::AddChild(GameObject* child) {
    if (child && child != this) {
        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        child->parent = this;
        children.push_back(child);
    }
}

void GameObject::RemoveChild(GameObject* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void GameObject::SetParent(GameObject* newParent) {
    if (newParent) {
        newParent->AddChild(this);
    }
    else if (parent) {
        parent->RemoveChild(this);
    }
}

void GameObject::InsertChildAt(GameObject* child, int index) {
    if (child && child != this) {
        if (child->parent) {
            child->parent->RemoveChild(child);
        }

        child->parent = this;

        if (index < 0) index = 0;
        if (index > static_cast<int>(children.size())) index = static_cast<int>(children.size());

        // Insert child
        children.insert(children.begin() + index, child);
    }
}

int GameObject::GetChildIndex(GameObject* child) const {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        return static_cast<int>(std::distance(children.begin(), it));
    }
    return -1;
}

void GameObject::Update() {
    // Si este GameObject está marcado para eliminación, no actualizar
    if (markedForDeletion) {
        return;
    }

    //if (!active) return;

    for (auto* component : components) {
        if (component->IsActive()) {
            component->Update();
        }

        // Si durante el Update del componente se marcó para eliminación, detener
        if (markedForDeletion) {
            return;
        }
    }

    // Crear copia de children para iterar de forma segura
    std::vector<GameObject*> childrenCopy = children;

    for (auto* child : childrenCopy) {
        // Verificar que el hijo todavía es válido y no está marcado para eliminación
        if (child && !child->IsMarkedForDeletion()) {
            child->Update();
        }
    }
}

void GameObject::FixedUpdate() {
    // Si este GameObject está marcado para eliminación, no actualizar
    if (markedForDeletion) {
        return;
    }

    //if (!active) return;

    for (auto* component : components) {
        
        if (component->IsActive()) {
            component->FixedUpdate();
        }

        // Si durante el Update del componente se marcó para eliminación, detener
        if (markedForDeletion) {
            return;
        }
    }

    // Crear copia de children para iterar de forma segura
    std::vector<GameObject*> childrenCopy = children;

    for (auto* child : childrenCopy) {
        
        // Verificar que el hijo todavía es válido y no está marcado para eliminación
        if (child && !child->IsMarkedForDeletion()) {
            child->FixedUpdate();
        }
    }
}

void GameObject::Serialize(nlohmann::json& gameObjectArray) const {
    // Create a JSON object
    nlohmann::json gameObjectObj;

    // Set the name and active state
    gameObjectObj["name"] = name;
    gameObjectObj["active"] = active;

    // Components
    nlohmann::json componentsArray = nlohmann::json::array();

    for (const auto* component : components) {
        nlohmann::json componentObj;
        componentObj["type"] = static_cast<int>(component->GetType());
        componentObj["active"] = component->IsActive();
        component->Serialize(componentObj);
        componentsArray.push_back(componentObj);
    }

    gameObjectObj["components"] = componentsArray;

    // Children
    nlohmann::json childrenArray = nlohmann::json::array();
    for (const auto* child : children) {
        child->Serialize(childrenArray);
    }
    gameObjectObj["children"] = childrenArray;

    gameObjectArray.push_back(gameObjectObj);
}

GameObject* GameObject::Deserialize(const nlohmann::json& gameObjectObj, GameObject* parent) {
    if (!gameObjectObj.is_object()) {
        return nullptr;
    }

    // Create gameobject
    std::string objName = gameObjectObj.contains("name") ? gameObjectObj["name"].get<std::string>() : "GameObject";
    GameObject* newObject = new GameObject(objName);

    if (gameObjectObj.contains("active")) {
        newObject->SetActive(gameObjectObj["active"].get<bool>());
    }

    if (parent) {
        parent->AddChild(newObject);
    }

    // Components
    if (gameObjectObj.contains("components") && gameObjectObj["components"].is_array()) {
        const nlohmann::json& componentsArray = gameObjectObj["components"];

        for (const auto& componentObj : componentsArray) {
            if (!componentObj.contains("type")) continue;

            ComponentType type = static_cast<ComponentType>(componentObj["type"].get<int>());

            Component* component = nullptr;
            if (type == ComponentType::TRANSFORM) {
                component = newObject->GetComponent(ComponentType::TRANSFORM);
            }
            else {
                component = newObject->CreateComponent(type);
            }

            if (component) {
                if (componentObj.contains("active")) {
                    component->SetActive(componentObj["active"].get<bool>());
                }
                component->Deserialize(componentObj);
            }
        }
    }

    // Children
    if (gameObjectObj.contains("children") && gameObjectObj["children"].is_array()) {
        const nlohmann::json& childrenArray = gameObjectObj["children"];
        for (const auto& childObj : childrenArray) {
            Deserialize(childObj, newObject);
        }
    }
    return newObject;
}


void GameObject::PublishGameObjectEvent(GameObjectEvent event, Component* newComponent)
{
    for (Component* component : components)
    {
        if (component)
        {
            component->OnGameObjectEvent(event, newComponent);
        }
    }
}