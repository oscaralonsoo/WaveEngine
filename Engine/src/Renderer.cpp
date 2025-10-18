#include "Renderer.h"
#include "Application.h"
#include <glad/glad.h>
#include <iostream>
#include "Texture.h"
#include "Shaders.h"
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"

Renderer::Renderer()
{
    LOG("Renderer Constructor");
    camera = make_unique<Camera>();
}

Renderer::~Renderer()
{
}

bool Renderer::Start()
{
    LOG("Initializing Renderer");

    // Create default shader
    defaultShader = make_unique<Shader>();

    if (!defaultShader->Create())
    {
        LOG("Failed to create default shader");
        return false;
    }

    // Crear textura checkerboard
    checkerTexture = make_unique<Texture>();
    checkerTexture->CreateCheckerboard();
    LOG("Checkerboard texture created");

    LOG("Renderer initialized successfully");

    // Para pruebas
    sphere = Primitives::CreateSphere();
    LoadMesh(sphere);
    cylinder = Primitives::CreateCylinder();
    LoadMesh(cylinder);
    pyramid = Primitives::CreatePyramid();
    LoadMesh(pyramid);

    return true;
}

void Renderer::LoadMesh(Mesh& mesh)
{
    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // VBO for vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.num_vertices * 3 * sizeof(float), mesh.vertices, GL_STATIC_DRAW);

    // EBO for indices
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.num_indices * sizeof(unsigned int), mesh.indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Save IDs in mesh
    mesh.id_vertex = VBO;
    mesh.id_index = EBO;
    mesh.id_VAO = VAO;

    glBindVertexArray(0);
}

void Renderer::DrawMesh(const Mesh& mesh)
{
    if (mesh.id_VAO == 0)
    {
        LOG("ERROR: Trying to draw mesh without VAO");
        return;
    }

    glBindVertexArray(mesh.id_VAO);
    glDrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Renderer::UnloadMesh(Mesh& mesh)
{
    if (mesh.id_VAO != 0)
    {
        glDeleteVertexArrays(1, &mesh.id_VAO);
        mesh.id_VAO = 0;
    }

    if (mesh.id_vertex != 0)
    {
        glDeleteBuffers(1, &mesh.id_vertex);
        mesh.id_vertex = 0;
    }

    if (mesh.id_index != 0)
    {
        glDeleteBuffers(1, &mesh.id_index);
        mesh.id_index = 0;
    }
}

bool Renderer::PreUpdate()
{
    return true;
}

bool Renderer::Update()
{
    defaultShader->Use();

    // Actualizar camara
    camera->Update();

    // Enviar matrices al shader
    glUniformMatrix4fv(glGetUniformLocation(defaultShader->GetProgramID(), "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(defaultShader->GetProgramID(), "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    // Activar y bindear la textura
    glActiveTexture(GL_TEXTURE0);
    checkerTexture->Bind();

    // Habilitar texturizado
    glEnable(GL_TEXTURE_2D);

    // Renderizar todos los meshes cargados desde FileSystem
    const vector<Mesh>& meshes = Application::GetInstance().filesystem->GetMeshes();

    if (!meshes.empty())
    {
        // Si hay meshes cargados, dibujarlos
        for (const auto& mesh : meshes)
        {
            DrawMesh(mesh);
        }
    }
    else
    {
        // Si no hay meshes, mostrar la piramide por defecto
        DrawMesh(pyramid);
    }

    checkerTexture->Unbind();

    return true;
}

bool Renderer::CleanUp()
{
    LOG("Cleaning up Renderer");

    if (defaultShader)
    {
        defaultShader->Delete();
    }

    return true;
}