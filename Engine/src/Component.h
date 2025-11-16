#pragma once
#include <string>

class GameObject;

enum class ComponentType {
    TRANSFORM,
    MESH,
    MATERIAL,
    CAMERA,
    UNKNOWN
};

class Component {
public:

    Component(GameObject* owner, ComponentType type);
    virtual ~Component() = default;  

    virtual void Enable() {};
    virtual void Update() {};
    virtual void Disable() {};
    virtual void OnEditor() {};  

    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }
    void SetActive(bool active) { this->active = active; }

public:
    GameObject* owner;      
    ComponentType type;     
    bool active = true;     
    std::string name;      
};