#pragma once

#include "Component.h"
#include "ModuleLoader.h"  
#include "ModuleResources.h"  
#include <glm/glm.hpp>

class ComponentMesh : public Component {
public:
    // Constructor and destructor
    ComponentMesh(GameObject* owner);
    ~ComponentMesh();

    // Component lifecycle
    void Update() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    bool IsType(ComponentType type) override { return type == ComponentType::MESH; };
    bool IsIncompatible(ComponentType type) override { return type == ComponentType::MESH /*|| type == ComponentType::SkinnedMeshRenderer*/; };

    // Load mesh from resource system by UID
    bool LoadMeshByUID(UID uid);

    // Set mesh data directly (for primitives that don't use resource system)
    void SetMesh(const Mesh& meshData);

    // Accessors for mesh
    const Mesh& GetMesh() const;
    Mesh& GetMesh();

    // Validation
    bool HasMesh() const;

    // Draw the mesh
    void Draw();

    // Get mesh UID (0 if using direct mesh)
    UID GetMeshUID() const { return meshUID; }

    // Check if using resource system or direct mesh
    bool IsUsingResourceMesh() const { return meshUID != 0; }
    bool IsUsingDirectMesh() const { return hasDirectMesh && meshUID == 0; }

    // Primitive Type (for serialization)
    void SetPrimitiveType(const std::string& type) { primitiveType = type; }
    const std::string& GetPrimitiveType() const { return primitiveType; }

    // Mesh statistics
    unsigned int GetNumVertices() const {
        const Mesh& m = GetMesh();
        return static_cast<unsigned int>(m.vertices.size());
    }
    
    std::vector<Vertex> GetVertices() const {
        const Mesh& m = GetMesh();
        return m.vertices;
    }

    unsigned int GetNumIndices() const {
        const Mesh& m = GetMesh();
        return static_cast<unsigned int>(m.indices.size());
    }

    std::vector<unsigned int> GetIndices() const {
        const Mesh& m = GetMesh();
        return m.indices;
    }


    unsigned int GetNumTriangles() const {
        return GetNumIndices() / 3;
    }

    unsigned int GetNumTextures() const {
        const Mesh& m = GetMesh();
        return static_cast<unsigned int>(m.textures.size());
    }


    // AABB methods
    glm::vec3 GetAABBMin() const;
    glm::vec3 GetAABBMax() const;

    // World space AABB (transformed by GameObject's global matrix)
    void GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const;

private:

    UID meshUID = 0;
    // Release current mesh resource
    void ReleaseCurrentMesh();

private:

    // Direct mesh (for primitives)
    Mesh directMesh;                        // Direct mesh data (for primitives)
    bool hasDirectMesh;                     // True if using direct mesh instead of resource
    std::string primitiveType;              // Type of primitive for serialization
};