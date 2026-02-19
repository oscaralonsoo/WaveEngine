#pragma once

#include "ModuleResources.h"
#include <nlohmann/json.hpp>

class GameObject;

class ResourcePrefab : public Resource {
public:
    ResourcePrefab(UID uid);
    ~ResourcePrefab();

    // Override Resource methods
    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // Prefab-specific methods
    const nlohmann::json& GetPrefabData() const { return prefabData; }
    GameObject* Instantiate(); // Create instance in scene

private:
    nlohmann::json prefabData;
};