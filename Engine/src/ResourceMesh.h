#pragma once

#include "ModuleResources.h"
#include "FileSystem.h"  // Para struct Mesh

class ResourceMesh : public Resource {
public:
    ResourceMesh(UID uid);
    virtual ~ResourceMesh();

    bool LoadInMemory() override;
    void UnloadFromMemory() override;

    // getters
    const Mesh& GetMesh() const { return mesh; }
    unsigned int GetNumVertices() const { return mesh.vertices.size(); }
    unsigned int GetNumIndices() const { return mesh.indices.size(); }
    unsigned int GetNumTriangles() const { return mesh.indices.size() / 3; }

private:
    Mesh mesh;  
};