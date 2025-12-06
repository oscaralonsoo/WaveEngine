#pragma once

#include "Module.h"
#include <map>
#include <string>

// Resource UIDs
typedef unsigned long long UID;

// Base class for all resources
class Resource {
public:
    enum Type {
        TEXTURE,
        MESH,
        MODEL,
        MATERIAL,
        ANIMATION,
        UNKNOWN
    };

    Resource(UID uid, Type type);
    virtual ~Resource();

    // Getters
    Type GetType() const { return type; }
    UID GetUID() const { return uid; }
    const char* GetAssetFile() const { return assetsFile.c_str(); }
    const char* GetLibraryFile() const { return libraryFile.c_str(); }
    bool IsLoadedToMemory() const { return loadedInMemory; }
    unsigned int GetReferenceCount() const { return referenceCount; }

    // Load into memory (subclass implementation)
    virtual bool LoadInMemory() = 0;

    // Unload from memory
    virtual void UnloadFromMemory() = 0;

protected:
    UID uid = 0;
    std::string assetsFile;      // Path in Assets/
    std::string libraryFile;     // Path in Library/
    Type type = UNKNOWN;
    unsigned int referenceCount = 0;
    bool loadedInMemory = false;

    friend class ModuleResources;
};

// Resource management module
class ModuleResources : public Module {
public:
    ModuleResources();
    ~ModuleResources();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    // Find UID from Assets/ file
    UID Find(const char* fileInAssets) const;

    // Import new file and return its UID
    UID ImportFile(const char* newFileInAssets);

    // Generate unique UID
    UID GenerateNewUID();

    // Request resource (increment ref count)
    const Resource* RequestResource(UID uid) const;
    Resource* RequestResource(UID uid);

    // Release resource (decrement ref count)
    void ReleaseResource(UID uid);

    void ModuleResources::LoadResourcesFromMetaFiles();
    Resource* ModuleResources::CreateNewResourceWithUID(const char* assetsFile, Resource::Type type, UID uid);
    bool ModuleResources::GetResourceInfo(UID uid, std::string& outAssetPath, std::string& outLibraryPath);

    // Get resource type from file extension
    Resource::Type GetResourceTypeFromExtension(const std::string& extension) const;
    void RemoveResource(UID uid);

private:
    // Create new resource by type
    Resource* CreateNewResource(const char* assetsFile, Resource::Type type);

    // Generate Library/ path for resource
    std::string GenerateLibraryPath(Resource* resource);

    // Load resource from Library/ into memory
    Resource* LoadResourceFromLibrary(UID uid);

    // Save resource metadata
    void SaveResourceMetadata(Resource* resource);

    // Load resource metadata
    bool LoadResourceMetadata(UID uid);

    // Type-specific import methods
    bool ImportTexture(Resource* resource, const std::string& assetPath);
    bool ImportMesh(Resource* resource, const std::string& assetPath);
    bool ImportModel(Resource* resource, const std::string& assetPath);

private:
    std::map<UID, Resource*> resources;  // UID -> Resource* map
    UID nextUID = 1;                     // UID counter
};