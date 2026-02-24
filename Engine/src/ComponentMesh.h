#pragma once

#include "Component.h"
#include "ModuleResources.h"  
#include "ResourceMesh.h"
#include <glm/glm.hpp>

#include "AABB.h"

class ComponentMaterial;
class AABB;

class ComponentMesh : public Component {
public:
    // Constructor and destructor
    ComponentMesh(GameObject* owner, ComponentType type = ComponentType::MESH);
    ~ComponentMesh();

    // Component lifecycle
    void Update() override { cachedBones = false; }

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    virtual bool IsType(ComponentType type) override { return type == ComponentType::MESH; };
    virtual bool IsIncompatible(ComponentType type) override { return type == ComponentType::MESH || type == ComponentType::SKINNED_MESH; };

    // Load mesh from resource system by UID
    bool LoadMeshByUID(UID uid);

    // Set mesh data directly (for primitives that don't use resource system)
    virtual void SetMesh(const Mesh& meshData);

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

    // Attached Material
    ComponentMaterial* GetAttachedMaterial() { return attachedMaterial; }

    // AABB methods
    const AABB& GetAABB() const;
    const AABB& GetGlobalAABB() const;
    void UpdateStaticAABB();
    virtual void UpdateDynamicAABB() {}

    //SKINNING
    virtual void UpdateSkinningMatrices() {}

    virtual bool HasSkinning() const { return false; }
    virtual const std::vector<glm::mat4>& GetBoneMatrices() const { static std::vector<glm::mat4> empty; return empty; }

    //DEBUG
    void SetDrawStencil(bool b) { drawStencil = b; };
    void SetDrawMesh(bool b) { drawMesh = b; };
    void SetDrawNormals(bool b) { drawNormals = b; };
    bool GetDrawStencil() { return drawStencil; }
    bool GetDrawMesh() { return drawMesh; }
    bool GetDrawNormals() { return drawNormals; }

private:
    void OnGameObjectEvent(GameObjectEvent event, Component* component);


protected:

    UID meshUID = 0;
    void ReleaseCurrentMesh();

    Mesh directMesh;
    bool hasDirectMesh;
    std::string primitiveType;       
    
    AABB staticAABB;
    AABB dynamicAABB;

    //MATERIAL
    ComponentMaterial* attachedMaterial;

    //ANIMATION
    bool cachedBones = false;   

    //DEBUG
    bool drawStencil = false;
    bool drawMesh = false;
    bool drawNormals = false;
};