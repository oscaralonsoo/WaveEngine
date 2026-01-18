#include "ResourcePrefab.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleScene.h"
#include "Log.h"
#include <fstream>

ResourcePrefab::ResourcePrefab(UID uid)
    : Resource(uid, Resource::PREFAB)
{
}

ResourcePrefab::~ResourcePrefab()
{
    if (loadedInMemory) {
        UnloadFromMemory();
    }
}

bool ResourcePrefab::LoadInMemory()
{
    if (loadedInMemory) {
        LOG_DEBUG("[ResourcePrefab] Prefab already loaded: %llu", uid);
        return true;
    }

    if (assetsFile.empty()) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: No asset file path set for prefab UID %llu", uid);
        return false;
    }

    if (!std::filesystem::exists(assetsFile)) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Prefab file not found: %s", assetsFile.c_str());
        return false;
    }

    // Read prefab JSON
    std::ifstream file(assetsFile);
    if (!file.is_open()) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Cannot open prefab file: %s", assetsFile.c_str());
        return false;
    }

    try {
        file >> prefabData;
        file.close();
    }
    catch (const std::exception& e) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Failed to parse prefab JSON: %s", e.what());
        file.close();
        return false;
    }

    loadedInMemory = true;

    LOG_DEBUG("[ResourcePrefab] Prefab loaded into memory: %s (UID: %llu)",
        assetsFile.c_str(), uid);

    return true;
}

void ResourcePrefab::UnloadFromMemory()
{
    if (!loadedInMemory) {
        return;
    }

    prefabData.clear();
    loadedInMemory = false;

    LOG_DEBUG("[ResourcePrefab] Prefab unloaded from memory: UID %llu", uid);
}

GameObject* ResourcePrefab::Instantiate()
{
    if (!loadedInMemory) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Prefab not loaded in memory");
        return nullptr;
    }

    if (prefabData.empty()) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Prefab data is empty");
        return nullptr;
    }

    // Deserialize GameObject from JSON
    GameObject* instance = GameObject::Deserialize(prefabData, nullptr);

    if (!instance) {
        LOG_CONSOLE("[ResourcePrefab] ERROR: Failed to instantiate prefab");
        return nullptr;
    }

    // Add to scene root
    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root) {
        root->AddChild(instance);
    }

    LOG_DEBUG("[ResourcePrefab] Instantiated prefab: %s", instance->GetName().c_str());

    return instance;
}