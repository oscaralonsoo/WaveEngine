#include "Prefab.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleScene.h"
#include "Log.h"
#include <fstream>

Prefab::Prefab(const std::string& name)
    : name(name), isValid(false) {
}

bool Prefab::SaveFromGameObject(GameObject* source, const std::string& filepath) {
    if (!source) {
        LOG_CONSOLE("[Prefab] ERROR: Source GameObject is null");
        return false;
    }

    // Serialize the entire hierarchy
    nlohmann::json rootArray = nlohmann::json::array();
    source->Serialize(rootArray);

    if (rootArray.empty()) {
        LOG_CONSOLE("[Prefab] ERROR: Failed to serialize GameObject");
        return false;
    }

    // Store the complete hierarchy (entire array, not just first element)
    prefabData = rootArray;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("[Prefab] ERROR: Cannot create file: %s", filepath.c_str());
        return false;
    }

    file << prefabData.dump(4);
    file.close();

    isValid = true;
    LOG_CONSOLE("[Prefab] Saved: %s with %zu objects", name.c_str(), rootArray.size());
    return true;
}

GameObject* Prefab::Instantiate() {
    if (!isValid) {
        LOG_CONSOLE("[Prefab] ERROR: Prefab is not valid");
        return nullptr;
    }

    // Check if prefabData is an array (hierarchy) or single object
    GameObject* instance = nullptr;

    if (prefabData.is_array() && !prefabData.empty()) {
        // Deserialize the root object (first in array)
        instance = GameObject::Deserialize(prefabData[0], nullptr);

        if (!instance) {
            LOG_CONSOLE("[Prefab] ERROR: Failed to instantiate root object");
            return nullptr;
        }

        // Deserialize children if there are more objects in the array
        for (size_t i = 1; i < prefabData.size(); ++i) {
            GameObject* child = GameObject::Deserialize(prefabData[i], instance);
            if (child) {
                // Child should already be added to parent during Deserialize
                LOG_DEBUG("[Prefab] Deserialized child: %s", child->GetName().c_str());
            }
        }
    }
    else {
        // Old format: single object
        instance = GameObject::Deserialize(prefabData, nullptr);
    }

    if (!instance) {
        LOG_CONSOLE("[Prefab] ERROR: Failed to instantiate");
        return nullptr;
    }

    GameObject* root = Application::GetInstance().scene->GetRoot();
    if (root) {
        root->AddChild(instance);
    }

    LOG_DEBUG("[Prefab] Instantiated: %s", name.c_str());
    return instance;
}

bool Prefab::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("[Prefab] ERROR: Cannot open file: %s", filepath.c_str());
        return false;
    }

    try {
        file >> prefabData;
        isValid = true;

        if (prefabData.is_array()) {
            LOG_CONSOLE("[Prefab] Loaded: %s (%zu objects)", name.c_str(), prefabData.size());
        }
        else {
            LOG_CONSOLE("[Prefab] Loaded: %s (single object - old format)", name.c_str());
        }

        return true;
    }
    catch (const std::exception& e) {
        LOG_CONSOLE("[Prefab] ERROR: Failed to parse: %s", e.what());
        isValid = false;
        return false;
    }
}