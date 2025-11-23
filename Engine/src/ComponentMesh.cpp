#include "ComponentMesh.h"
#include "GameObject.h"
#include "Application.h"
#include "Transform.h"
#include "Frustum.h"
#include <limits>
#include <algorithm>  
#include <glm/glm.hpp>

ComponentMesh::ComponentMesh(GameObject* owner)
    : Component(owner, ComponentType::MESH),
    aabbMin(0.0f), aabbMax(0.0f)
{
}

ComponentMesh::~ComponentMesh()
{
    if (HasMesh())
    {
        Application::GetInstance().renderer->UnloadMesh(mesh);
    }
}

void ComponentMesh::Update()
{
    // Reserved for future mesh updates
}

void ComponentMesh::OnEditor()
{
    // Reserved for ImGui editor interface
}

void ComponentMesh::SetMesh(const Mesh& meshData)
{
    // Unload previous mesh if exists
    if (HasMesh())
    {
        Application::GetInstance().renderer->UnloadMesh(mesh);
    }

    // Copy mesh data
    mesh.vertices = meshData.vertices;
    mesh.indices = meshData.indices;
    mesh.textures = meshData.textures;

    // Reset OpenGL handles (will be set by renderer)
    mesh.VAO = 0;
    mesh.VBO = 0;
    mesh.EBO = 0;

    // Calculate AABB
    CalculateAABB();

    // Upload to GPU
    Application::GetInstance().renderer->LoadMesh(mesh);
}

void ComponentMesh::CalculateAABB()
{
    // If the mesh has no vertices, set the AABB to zero
    if (mesh.vertices.empty())
    {
        aabbMin = glm::vec3(0.0f);
        aabbMax = glm::vec3(0.0f);
        return;
    }

    // Initialize min and max with the first vertex position
    aabbMin = mesh.vertices[0].position;
    aabbMax = mesh.vertices[0].position;

    // Iterate through all vertices to find the minimum and maximum coordinates
    for (const auto& vertex : mesh.vertices)
    {
        aabbMin.x = std::min(aabbMin.x, vertex.position.x);
        aabbMin.y = std::min(aabbMin.y, vertex.position.y);
        aabbMin.z = std::min(aabbMin.z, vertex.position.z);

        aabbMax.x = std::max(aabbMax.x, vertex.position.x);
        aabbMax.y = std::max(aabbMax.y, vertex.position.y);
        aabbMax.z = std::max(aabbMax.z, vertex.position.z);
    }
}

void ComponentMesh::GetWorldAABB(glm::vec3& outMin, glm::vec3& outMax) const
{
    // Get the transform component to access the global transformation matrix
    Transform* transform = static_cast<Transform*>(
        owner->GetComponent(ComponentType::TRANSFORM));

    if (!transform)
    {
        // If no transform, return local AABB
        outMin = aabbMin;
        outMax = aabbMax;
        return;
    }

    // Get the global transformation matrix
    const glm::mat4& globalMatrix = transform->GetGlobalMatrix();

    // Transform all 8 corners of the local AABB to world space
    glm::vec3 corners[8];
    Frustum::GetAABBVertices(aabbMin, aabbMax, corners);

    // Transform the first corner and initialize world min/max
    glm::vec4 worldCorner = globalMatrix * glm::vec4(corners[0], 1.0f);
    outMin = glm::vec3(worldCorner);
    outMax = glm::vec3(worldCorner);

    // Transform remaining corners and expand the world AABB
    for (int i = 1; i < 8; i++)
    {
        worldCorner = globalMatrix * glm::vec4(corners[i], 1.0f);
        glm::vec3 corner3D = glm::vec3(worldCorner);

        outMin.x = std::min(outMin.x, corner3D.x);
        outMin.y = std::min(outMin.y, corner3D.y);
        outMin.z = std::min(outMin.z, corner3D.z);

        outMax.x = std::max(outMax.x, corner3D.x);
        outMax.y = std::max(outMax.y, corner3D.y);
        outMax.z = std::max(outMax.z, corner3D.z);
    }
}