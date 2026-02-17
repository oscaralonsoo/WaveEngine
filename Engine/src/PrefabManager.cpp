#include "PrefabManager.h"
#include "Prefab.h"
#include "Log.h"

PrefabManager& PrefabManager::GetInstance() {
    static PrefabManager instance;
    return instance;
}

bool PrefabManager::LoadPrefab(const std::string& name, const std::string& filepath) {
    auto prefab = std::make_unique<Prefab>(name);

    if (!prefab->LoadFromFile(filepath)) {
        LOG_CONSOLE("[PrefabManager] ERROR: Failed to load prefab: %s", name.c_str());
        return false;
    }

    prefabs[name] = std::move(prefab);
    LOG_CONSOLE("[PrefabManager] Loaded prefab: %s", name.c_str());

    return true;
}

GameObject* PrefabManager::InstantiatePrefab(const std::string& name) {
    auto it = prefabs.find(name);
    if (it == prefabs.end()) {
        LOG_CONSOLE("[PrefabManager] ERROR: Prefab not found: %s", name.c_str());
        return nullptr;
    }

    return it->second->Instantiate();
}

bool PrefabManager::CreatePrefab(const std::string& name, GameObject* source, const std::string& filepath) {
    if (!source) {
        LOG_CONSOLE("[PrefabManager] ERROR: Source GameObject is null");
        return false;
    }

    auto prefab = std::make_unique<Prefab>(name);

    if (!prefab->SaveFromGameObject(source, filepath)) {
        LOG_CONSOLE("[PrefabManager] ERROR: Failed to save prefab: %s", name.c_str());
        return false;
    }

    prefabs[name] = std::move(prefab);
    LOG_CONSOLE("[PrefabManager] Created prefab: %s", name.c_str());

    return true;
}

bool PrefabManager::HasPrefab(const std::string& name) const {
    return prefabs.find(name) != prefabs.end();
}

void PrefabManager::Clear() {
    prefabs.clear();
    LOG_CONSOLE("[PrefabManager] Cleared all prefabs");
}