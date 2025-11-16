#pragma once

#include "Component.h"
#include "FileSystem.h" 
#include <glm/glm.hpp>

class ComponentMesh : public Component {
public:
    // Constructor and destructor
    ComponentMesh(GameObject* owner);
    ~ComponentMesh();

    // Component lifecycle
    void Update() override;
    void OnEditor() override;

    // Set mesh data and upload it to GPU
    void SetMesh(const Mesh& meshData);

    // Accessors for mesh
    const Mesh& GetMesh() const { return mesh; }
    Mesh& GetMesh() { return mesh; }

    // Validation
    bool HasMesh() const { return mesh.IsValid(); }

    // Mesh statistics
    unsigned int GetNumVertices() const { return static_cast<unsigned int>(mesh.vertices.size()); }
    unsigned int GetNumIndices() const { return static_cast<unsigned int>(mesh.indices.size()); }
    unsigned int GetNumTriangles() const { return GetNumIndices() / 3; }
    unsigned int GetNumTextures() const { return static_cast<unsigned int>(mesh.textures.size()); }

    // Local space Axis-Aligned Bounding Box (AABB) accessors
    glm::vec3 GetAABBMin() const { return aabbMin; }
    glm::vec3 GetAABBMax() const { return aabbMax; }

    // World space AABB (transformed by GameObject's global matrix)
    void GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const;

private:
    Mesh mesh;                 // Mesh data

    // Local space bounding box
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    // Calculate the AABB from mesh vertices
    void CalculateAABB();
};