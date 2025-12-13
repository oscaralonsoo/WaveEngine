#include "ComponentMesh.h"
#include "GameObject.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ResourceMesh.h"
#include "Transform.h"
#include "Log.h"
#include <glad/glad.h>

ComponentMesh::ComponentMesh(GameObject* owner)
    : Component(owner, ComponentType::MESH),
    meshUID(0),
    hasDirectMesh(false)
{
}

ComponentMesh::~ComponentMesh()
{
    ReleaseCurrentMesh();

    // Clean up direct mesh GPU resources if present
    if (hasDirectMesh && directMesh.VAO != 0) {
        glDeleteVertexArrays(1, &directMesh.VAO);
        glDeleteBuffers(1, &directMesh.VBO);
        glDeleteBuffers(1, &directMesh.EBO);
    }
}

void ComponentMesh::Update()
{
}

void ComponentMesh::OnEditor()
{
}

void ComponentMesh::ReleaseCurrentMesh()
{
    if (meshUID != 0) {
        Application::GetInstance().resources->ReleaseResource(meshUID);
        meshUID = 0;
    }

    hasDirectMesh = false;
}

bool ComponentMesh::LoadMeshByUID(UID meshUID)
{
    if (meshUID == 0)
    {
        LOG_CONSOLE("ERROR: Invalid mesh UID (0)");
        return false;
    }

    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources)
    {
        LOG_CONSOLE("ERROR: ModuleResources not available");
        return false;
    }

    Resource* resource = resources->RequestResource(meshUID);

    if (!resource)
    {
        LOG_CONSOLE("ERROR: Failed to load mesh resource with UID: %llu", meshUID);
        return false;
    }

    if (resource->GetType() != Resource::MESH)
    {
        LOG_CONSOLE("ERROR: Resource UID %llu is not a mesh", meshUID);
        return false;
    }

    ResourceMesh* meshResource = static_cast<ResourceMesh*>(resource);

    const Mesh& loadedMesh = meshResource->GetMesh();

    if (loadedMesh.vertices.empty() || loadedMesh.indices.empty())
    {
        LOG_CONSOLE("ERROR: Loaded mesh is empty (UID: %llu)", meshUID);
        return false;
    }

    // Assign mesh to component
    SetMesh(loadedMesh);

    // Store UID for reference
    this->meshUID = meshUID;

    return true;
}

void ComponentMesh::SetMesh(const Mesh& mesh)
{
    // Release resource system mesh if any
    ReleaseCurrentMesh();

    // Copy mesh data for direct storage
    directMesh = mesh;
    hasDirectMesh = true;

    // Upload mesh to GPU if data is available
    if (!directMesh.vertices.empty() && !directMesh.indices.empty())
    {
        // Generate OpenGL buffers
        glGenVertexArrays(1, &directMesh.VAO);
        glGenBuffers(1, &directMesh.VBO);
        glGenBuffers(1, &directMesh.EBO);

        glBindVertexArray(directMesh.VAO);

        // Upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, directMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER,
            directMesh.vertices.size() * sizeof(Vertex),
            directMesh.vertices.data(),
            GL_STATIC_DRAW);

        // Upload index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, directMesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            directMesh.indices.size() * sizeof(unsigned int),
            directMesh.indices.data(),
            GL_STATIC_DRAW);

        // Configure vertex attributes
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, normal));

        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            (void*)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
    }
}

const Mesh& ComponentMesh::GetMesh() const
{
    // Return resource mesh if loaded
    if (meshUID != 0) {
        // Retrieve resource without incrementing reference count
        const Resource* resource = Application::GetInstance().resources->GetResource(meshUID);

        if (resource && resource->IsLoadedToMemory()) {
            const ResourceMesh* meshResource = dynamic_cast<const ResourceMesh*>(resource);
            if (meshResource) {
                return meshResource->GetMesh();
            }
        }

        // Log warning if resource exists but is not loaded
        if (resource) {
            LOG_DEBUG("[ComponentMesh] WARNING: Resource UID %llu exists but not loaded in memory", meshUID);
        }
    }

    // Return direct mesh as fallback
    return directMesh;
}

Mesh& ComponentMesh::GetMesh()
{
    // Invoke const version and remove const qualifier from result
    return const_cast<Mesh&>(static_cast<const ComponentMesh*>(this)->GetMesh());
}

bool ComponentMesh::HasMesh() const
{
    // Verify resource mesh availability
    if (meshUID != 0) {
        const Resource* resource = Application::GetInstance().resources->GetResource(meshUID);

        if (resource && resource->IsLoadedToMemory()) {
            const ResourceMesh* meshResource = dynamic_cast<const ResourceMesh*>(resource);
            if (meshResource) {
                const Mesh& mesh = meshResource->GetMesh();
                return !mesh.vertices.empty() && !mesh.indices.empty();
            }
        }
    }

    // Verify direct mesh availability
    if (hasDirectMesh) {
        return !directMesh.vertices.empty() && !directMesh.indices.empty();
    }

    return false;
}

void ComponentMesh::Draw()
{
    if (!IsActive()) return;

    const Mesh* meshToDraw = nullptr;

    // Prioritize resource mesh
    if (meshUID != 0) {
        const Resource* resource = Application::GetInstance().resources->GetResource(meshUID);

        if (resource && resource->IsLoadedToMemory()) {
            const ResourceMesh* meshResource = dynamic_cast<const ResourceMesh*>(resource);
            if (meshResource) {
                meshToDraw = &meshResource->GetMesh();
            }
        }
    }

    // Use direct mesh as fallback
    if (!meshToDraw && hasDirectMesh) {
        meshToDraw = &directMesh;
    }

    // Render mesh if valid
    if (meshToDraw && meshToDraw->VAO != 0 && !meshToDraw->indices.empty()) {
        glBindVertexArray(meshToDraw->VAO);
        glDrawElements(GL_TRIANGLES, meshToDraw->indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

glm::vec3 ComponentMesh::GetAABBMin() const
{
    const Mesh& mesh = GetMesh();

    if (mesh.vertices.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 minBounds(std::numeric_limits<float>::max());

    for (const auto& vertex : mesh.vertices) {
        minBounds.x = std::min(minBounds.x, vertex.position.x);
        minBounds.y = std::min(minBounds.y, vertex.position.y);
        minBounds.z = std::min(minBounds.z, vertex.position.z);
    }

    return minBounds;
}

glm::vec3 ComponentMesh::GetAABBMax() const
{
    const Mesh& mesh = GetMesh();

    if (mesh.vertices.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& vertex : mesh.vertices) {
        maxBounds.x = std::max(maxBounds.x, vertex.position.x);
        maxBounds.y = std::max(maxBounds.y, vertex.position.y);
        maxBounds.z = std::max(maxBounds.z, vertex.position.z);
    }

    return maxBounds;
}

void ComponentMesh::GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const
{
    glm::vec3 localMin = GetAABBMin();
    glm::vec3 localMax = GetAABBMax();

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));

    if (!transform) {
        outMin = localMin;
        outMax = localMax;
        return;
    }

    // Transform all eight corners to world space
    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 corners[8] = {
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMin.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMin.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMin.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMin.x, localMax.y, localMax.z, 1.0f)),
        glm::vec3(globalMatrix * glm::vec4(localMax.x, localMax.y, localMax.z, 1.0f))
    };

    // Compute world-space AABB from transformed corners
    outMin = corners[0];
    outMax = corners[0];

    for (int i = 1; i < 8; ++i) {
        outMin = glm::min(outMin, corners[i]);
        outMax = glm::max(outMax, corners[i]);
    }
}

void ComponentMesh::Serialize(nlohmann::json& componentObj) const
{
    // UID
    if (meshUID != 0) {
        componentObj["meshUID"] = meshUID;
    }

    // Direct mesh
    if (hasDirectMesh && !primitiveType.empty()) {
        componentObj["primitiveType"] = primitiveType;
    }
}

void ComponentMesh::Deserialize(const nlohmann::json& componentObj)
{
    // UID
    if (componentObj.contains("meshUID")) {
        UID uid = componentObj["meshUID"].get<UID>();
        if (uid != 0) {
            LoadMeshByUID(uid);
            return;
        }
    }

    // Primitive Type
    if (componentObj.contains("primitiveType")) {
        std::string primType = componentObj["primitiveType"].get<std::string>();
        primitiveType = primType;

        Mesh recreatedMesh;
        if (primType == "Cube") {
            recreatedMesh = Primitives::CreateCube();
        } else if (primType == "Pyramid") {
            recreatedMesh = Primitives::CreatePyramid();
        } else if (primType == "Plane") {
            recreatedMesh = Primitives::CreatePlane();
        } else if (primType == "Sphere") {
            recreatedMesh = Primitives::CreateSphere();
        } else if (primType == "Cylinder") {
            recreatedMesh = Primitives::CreateCylinder();
        }

        SetMesh(recreatedMesh);
    }
}