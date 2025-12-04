#pragma once

#include "Module.h"
#include <map>
#include <string>

// Tipo para los UIDs de recursos
typedef unsigned long long UID;

// Clase base para todos los recursos
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

    // Carga en memoria (implementado por cada tipo de recurso)
    virtual bool LoadInMemory() = 0;

    // Descarga de memoria
    virtual void UnloadFromMemory() = 0;

protected:
    UID uid = 0;
    std::string assetsFile;      // Ruta en Assets/
    std::string libraryFile;     // Ruta en Library/
    Type type = UNKNOWN;
    unsigned int referenceCount = 0;
    bool loadedInMemory = false;

    friend class ModuleResources;
};

// Módulo central de gestión de recursos
class ModuleResources : public Module {
public:
    ModuleResources();
    ~ModuleResources();

    bool Awake() override;
    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    // Buscar UID de un archivo en Assets/
    UID Find(const char* fileInAssets) const;

    // Importar un nuevo archivo y devolverme su UID
    UID ImportFile(const char* newFileInAssets);

    // Generar nuevo UID único
    UID GenerateNewUID();

    // Solicitar un recurso (incrementa reference count)
    const Resource* RequestResource(UID uid) const;
    Resource* RequestResource(UID uid);

    // Liberar un recurso (decrementa reference count)
    void ReleaseResource(UID uid);

    void ModuleResources::LoadResourcesFromMetaFiles();
    Resource* ModuleResources::CreateNewResourceWithUID(const char* assetsFile, Resource::Type type, UID uid);
    bool ModuleResources::GetResourceInfo(UID uid, std::string& outAssetPath, std::string& outLibraryPath);
    // Obtener tipo de recurso según extensión
    Resource::Type GetResourceTypeFromExtension(const std::string& extension) const;

private:
    // Crear un nuevo recurso según su tipo
    Resource* CreateNewResource(const char* assetsFile, Resource::Type type);

    // Generar ruta en Library/ para un recurso
    std::string GenerateLibraryPath(Resource* resource);

    // Cargar un recurso desde Library/ a memoria
    Resource* LoadResourceFromLibrary(UID uid);

    // Guardar metadata de recurso
    void SaveResourceMetadata(Resource* resource);

    // Cargar metadata de recurso
    bool LoadResourceMetadata(UID uid);

    // Métodos de importación específicos
    bool ImportTexture(Resource* resource, const std::string& assetPath);
    bool ImportMesh(Resource* resource, const std::string& assetPath);
    bool ImportModel(Resource* resource, const std::string& assetPath);

private:
    std::map<UID, Resource*> resources;  // Mapa UID -> Resource*
    UID nextUID = 1;                     // Contador para generar UIDs
};