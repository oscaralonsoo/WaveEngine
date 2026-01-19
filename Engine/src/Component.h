#pragma once
#include <string>
#include <nlohmann/json.hpp>

class GameObject;

enum class ComponentType {
    TRANSFORM,
    MESH,
    MATERIAL,
    CAMERA,
    ROTATE,
    AUDIO_SOURCE,     
    AUDIO_LISTENER,   
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

    // Serialization
    virtual void Serialize(nlohmann::json& componentObj) const {};
    virtual void Deserialize(const nlohmann::json& componentObj) {};

    ComponentType GetType() const { return type; }
    bool IsActive() const { return active; }
    void SetActive(bool active) { this->active = active; }

public:
    GameObject* owner;
    ComponentType type;
    bool active = true;
    std::string name;
};