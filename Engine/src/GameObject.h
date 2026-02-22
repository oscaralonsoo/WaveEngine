#pragma once
#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class Component;
class Transform;
enum class ComponentType;

enum class GameObjectEvent {
    TRANSFORM_CHANGED,
    TRANSFORM_SCALED,
    COMPONENT_ADDED,
    COMPONENT_REMOVED,
    OBJECT_DESTROYED,
    HIERARCHY_CHANGED,
    MESH_CHANGED
};

class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    Component* CreateComponent(ComponentType type);
    void RemoveComponent(Component* comp);

    //QOL
    std::unique_ptr<Component> ExtractComponent(Component* comp);
    void ReinsertComponentAt(std::unique_ptr<Component> comp, int index);
    int GetComponentIndex(Component* comp) const;

    Component* GetComponent(ComponentType type) const;
    std::vector<Component*> GetComponentsOfType(ComponentType type) const;
    Component* GetComponentInChildren(ComponentType type);
    void GetComponentsInChildren(ComponentType type, std::vector<Component*>& outlist);
    Component* GetComponentInParent(ComponentType type);
    void GetComponentsInParent(ComponentType type, std::vector<Component*>& outlist);

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void SetParent(GameObject* newParent);
    void InsertChildAt(GameObject* child, int index);
    int GetChildIndex(GameObject* child) const;

    void Update();
    void FixedUpdate();

    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }
    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

    void MarkForDeletion() { markedForDeletion = true; }
    bool IsMarkedForDeletion() const { return markedForDeletion; }
    void MarkCleaning() { isCleaning = true; };
    bool IsCleaning() { return isCleaning; };

    // Serialization
    void Serialize(nlohmann::json& gameObjectArray) const;
    static GameObject* Deserialize(const nlohmann::json& gameObjectObj, GameObject* parent = nullptr);

    //INTERNAL EVENTS
    void PublishGameObjectEvent(GameObjectEvent event, Component* component = nullptr);

public:
    std::string name;
    bool active = true;
    Transform* transform = nullptr;

private:
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    std::vector<Component*> components;
    std::vector<std::unique_ptr<Component>> componentOwners;

    bool markedForDeletion = false;
    bool isCleaning = false;

};