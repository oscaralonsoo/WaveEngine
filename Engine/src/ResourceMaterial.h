#pragma once
#include "ModuleResources.h"

class Material;

class ResourceMaterial : public Resource {
public:
    ResourceMaterial(UID uid);
    virtual ~ResourceMaterial();

    virtual bool LoadInMemory() override = 0;
    virtual void UnloadFromMemory() override = 0;
    
    const Material* GetMaterial() { return material; }

private:
    
    Material* material;
};