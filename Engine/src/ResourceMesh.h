#pragma once

#include "ModuleResources.h"
#include "FileSystem.h"  // Para struct Mesh

class ResourceMesh : public Resource {
public:
    ResourceMesh(UID uid);
    virtual ~ResourceMesh();

    // Implementación de métodos virtuales
    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // Getters específicos de mesh
    const Mesh& GetMesh() const { return mesh; }
    unsigned int GetNumVertices() const { return mesh.vertices.size(); }
    unsigned int GetNumIndices() const { return mesh.indices.size(); }
    unsigned int GetNumTriangles() const { return mesh.indices.size() / 3; }

private:
    Mesh mesh;  // Datos de la malla
};