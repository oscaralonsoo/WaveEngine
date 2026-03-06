#include "ResourceMaterial.h"
#include "ModuleResources.h"
#include "Log.h"
#include <fstream>

ResourceMaterial::ResourceMaterial(UID uid)
    : Resource(uid, Resource::MATERIAL) {
}

ResourceMaterial::~ResourceMaterial() {
    UnloadFromMemory();
}

bool ResourceMaterial::LoadInMemory()
{
    return true;
}

void ResourceMaterial::UnloadFromMemory()
{

}