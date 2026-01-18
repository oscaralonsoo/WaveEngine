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

    nlohmann::json rootArray = nlohmann::json::array();
    source->Serialize(rootArray);

    if (rootArray.empty()) {
        LOG_CONSOLE("[Prefab] ERROR: Failed to serialize GameObject");
        return false;
    }

    prefabData = rootArray[0];

    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_CONSOLE("[Prefab] ERROR: Cannot create file: %s", filepath.c_str());
        return false;
    }

    file << prefabData.dump(4);
    file.close();

    isValid = true;
    LOG_CONSOLE("[Prefab] Saved: %s", name.c_str());
    return true;
}

GameObject* Prefab::Instantiate() {
    if (!isValid) {
        LOG_CONSOLE("[Prefab] ERROR: Prefab is not valid");
        return nullptr;
    }

    GameObject* instance = GameObject::Deserialize(prefabData, nullptr);
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
        LOG_CONSOLE("[Prefab] Loaded: %s", name.c_str());
        return true;
    }
    catch (const std::exception& e) {
        LOG_CONSOLE("[Prefab] ERROR: Failed to parse: %s", e.what());
        isValid = false;
        return false;
    }
}