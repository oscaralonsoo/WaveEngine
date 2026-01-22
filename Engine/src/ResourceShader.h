#pragma once

#include "ModuleResources.h"
#include "Shader.h"
#include <string>

class ResourceShader : public Resource {
public:
    ResourceShader(UID uid);
    virtual ~ResourceShader();

    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // Shader-specific
    bool Compile();
    
    void SetSourceCode(const std::string& source);
    const std::string& GetSourceCode() const { return sourceCode; }
    
    Shader* GetShader() { return shader; }

private:
    std::string sourceCode;
    Shader* shader = nullptr;
};
