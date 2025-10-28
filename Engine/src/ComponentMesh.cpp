#include "ComponentMesh.h"
#include "GameObject.h"
#include "Application.h"

ComponentMesh::ComponentMesh(GameObject* owner)
    : Component(owner, ComponentType::MESH)
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

}

void ComponentMesh::OnEditor()
{

}

void ComponentMesh::SetMesh(const Mesh& meshData)
{
    if (HasMesh())
    {
        Application::GetInstance().renderer->UnloadMesh(mesh);
    }

    mesh.vertices = meshData.vertices;
    mesh.indices = meshData.indices;
    mesh.textures = meshData.textures;

    mesh.VAO = 0;
    mesh.VBO = 0;
    mesh.EBO = 0;

    Application::GetInstance().renderer->LoadMesh(mesh);
}