#pragma once
#include <vector>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class Component;
class ModuleScripting;
enum class ComponentType;

class GameObject {
public:
    GameObject(const std::string& name = "GameObject");
    ~GameObject();

    Component* CreateComponent(ComponentType type);

    Component* GetComponent(ComponentType type) const;

    std::vector<Component*> GetComponentsOfType(ComponentType type) const;

    void AddChild(GameObject* child);
    void RemoveChild(GameObject* child);
    void SetParent(GameObject* newParent);
    void InsertChildAt(GameObject* child, int index);
    int GetChildIndex(GameObject* child) const;

    void Update();

    const std::string& GetName() const { return name; }
    void SetName(const std::string& newName) { name = newName; }
    bool IsActive() const { return active; }
    void SetActive(bool state) { active = state; }
    GameObject* GetParent() const { return parent; }
    const std::vector<GameObject*>& GetChildren() const { return children; }
    const std::vector<Component*>& GetComponents() const { return components; }

    void MarkForDeletion() { markedForDeletion = true; }
    bool IsMarkedForDeletion() const { return markedForDeletion; }

    // Serialization
    void Serialize(nlohmann::json& gameObjectArray) const;
    static GameObject* Deserialize(const nlohmann::json& gameObjectObj, GameObject* parent = nullptr);

public:
    std::string name;
    bool active = true;
    std::vector<ModuleScripting*> scripts;

private:
    GameObject* parent = nullptr;
    std::vector<GameObject*> children;

    std::vector<Component*> components;
    std::vector<std::unique_ptr<Component>> componentOwners;

    bool markedForDeletion = false;

};