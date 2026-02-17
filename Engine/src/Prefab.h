#pragma once
#include <string>
#include <nlohmann/json.hpp>

class GameObject;

class Prefab {
public:
    Prefab(const std::string& name);

    bool SaveFromGameObject(GameObject* source, const std::string& filepath);
    GameObject* Instantiate();
    bool LoadFromFile(const std::string& filepath);

    bool IsValid() const { return isValid; }

private:
    std::string name;
    nlohmann::json prefabData;
    bool isValid;
};