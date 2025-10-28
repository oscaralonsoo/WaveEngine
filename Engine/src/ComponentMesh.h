#pragma once

#include "Component.h"
#include "FileSystem.h" 

class ComponentMesh : public Component {
public:
    ComponentMesh(GameObject* owner);
    ~ComponentMesh();

    void Update() override;
    void OnEditor() override;

    void SetMesh(const Mesh& meshData);

    const Mesh& GetMesh() const { return mesh; }
    Mesh& GetMesh() { return mesh; }

    bool HasMesh() const { return mesh.VAO != 0; }

    // Helper functions
    unsigned int GetNumVertices() const { return static_cast<unsigned int>(mesh.vertices.size()); }
    unsigned int GetNumIndices() const { return static_cast<unsigned int>(mesh.indices.size()); }
    unsigned int GetNumTriangles() const { return static_cast<unsigned int>(mesh.indices.size() / 3); }
    unsigned int GetNumTextures() const { return static_cast<unsigned int>(mesh.textures.size()); }

private:
    Mesh mesh;
};