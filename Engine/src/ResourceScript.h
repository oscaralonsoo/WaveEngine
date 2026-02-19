#pragma once

#include "ModuleResources.h"
#include <string>

class ResourceScript : public Resource {
public:
    ResourceScript(UID uid);
    ~ResourceScript();

    // Override Resource methods
    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // Script-specific methods
    const std::string& GetScriptContent() const { return scriptContent; }
    bool Reload(); // Hot reloading

    long long GetLastLoadTime() const { return lastLoadTime; }
    bool NeedsReload() const;

private:
    std::string scriptContent;
    long long lastLoadTime;
};