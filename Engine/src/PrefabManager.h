#pragma once
#include <string>
#include <unordered_map>
#include <memory>

class Prefab;
class GameObject;

class PrefabManager {
public:
    static PrefabManager& GetInstance();

    bool LoadPrefab(const std::string& name, const std::string& filepath);
    GameObject* InstantiatePrefab(const std::string& name);
    bool CreatePrefab(const std::string& name, GameObject* source, const std::string& filepath);
    bool HasPrefab(const std::string& name) const;
    void Clear();

private:
    PrefabManager() = default;
    std::unordered_map<std::string, std::unique_ptr<Prefab>> prefabs;
};