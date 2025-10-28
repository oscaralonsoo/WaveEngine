#include "Renderer.h"
#include "Application.h"
#include <glad/glad.h>
#include <iostream>
#include "Texture.h"
#include "Shaders.h"
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"

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
    else
    {
        std::cout << "Shader created successfully!" << std::endl;
        std::cout << "Shader Program ID: " << defaultShader->GetProgramID() << std::endl;
    }
    // Create default texture
    defaultTexture = make_unique<Texture>();

    if (!defaultTexture->LoadFromFile("Assets\\Baker_house.png"))
    {
        LOG("Failed to load texture from file, using checkerboard");
        defaultTexture->CreateCheckerboard();
    }
    else
    {
        LOG("Texture loaded successfully from file");
    }

    LOG("Renderer initialized successfully");

    // for testing
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
    // Generate VAO
    glGenVertexArrays(1, &mesh.VAO);
    glBindVertexArray(mesh.VAO);

    // Generate and bind VBO for vertices
    glGenBuffers(1, &mesh.VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), &mesh.vertices[0], GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // Normal attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Texture coordinate attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Generate and bind EBO for indices
    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), &mesh.indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    LOG("Mesh loaded - VAO: %d, Vertices: %d, Indices: %d", mesh.VAO, mesh.vertices.size(), mesh.indices.size());
}

void Renderer::DrawMesh(const Mesh& mesh)
{
    if (mesh.VAO == 0)
    {
        LOG("ERROR: Trying to draw mesh without VAO");
        return;
    }

    glBindVertexArray(mesh.VAO);
    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    //std::cout << "=== LoadMesh DEBUG ===" << std::endl;
    //std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
    //std::cout << "Indices: " << mesh.indices.size() << std::endl;
    //std::cout << "VAO: " << mesh.VAO << std::endl;
    //std::cout << "VBO: " << mesh.VBO << std::endl;
    //std::cout << "EBO: " << mesh.EBO << std::endl;

}

void Renderer::UnloadMesh(Mesh& mesh)
{
    if (mesh.VAO != 0)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        mesh.VAO = 0;
    }

    if (mesh.VBO != 0)
    {
        glDeleteBuffers(1, &mesh.VBO);
        mesh.VBO = 0;
    }

    if (mesh.EBO != 0)
    {
        glDeleteBuffers(1, &mesh.EBO);
        mesh.EBO = 0;
    }
}

void Renderer::LoadTexture(const std::string& path)
{
    auto newTexture = make_unique<Texture>();

    if (newTexture->LoadFromFile(path))
    {
        defaultTexture = std::move(newTexture);
        std::cout << "Texture applied successfully: " << path << std::endl;
    }
    else
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
}

bool Renderer::PreUpdate()
{
    return true;
}

bool Renderer::Update()
{
    glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    defaultShader->Use();

    camera->Update();

    int width, height;
    Application::GetInstance().window->GetWindowSize(width, height);

    // Update aspect ratio 
    float aspectRatio = (float)width / (float)height;
    camera->SetAspectRatio(aspectRatio);

    GLuint shaderProgram = defaultShader->GetProgramID();

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    // Activate and bind default texture 
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (root != nullptr && root->GetChildren().size() > 0)
    {
        DrawScene();
    }

    defaultTexture->Unbind();

    return true;
}

bool Renderer::CleanUp()
{
    LOG("Cleaning up Renderer");

    // Unload test primitives
    UnloadMesh(sphere);
    UnloadMesh(cylinder);
    UnloadMesh(pyramid);

    if (defaultShader)
    {
        defaultShader->Delete();
    }

    return true;
}

void Renderer::DrawScene()
{
    GameObject* root = Application::GetInstance().scene->GetRoot();

    if (root == nullptr)
        return;

    DrawGameObjectRecursive(root);
}

void Renderer::DrawGameObject(GameObject* gameObject)
{
    if (gameObject == nullptr || !gameObject->IsActive())
        return;

    DrawGameObjectRecursive(gameObject);
}

void Renderer::DrawGameObjectRecursive(GameObject* gameObject)
{
    if (!gameObject->IsActive())
        return;

    Transform* transform = static_cast<Transform*>(gameObject->GetComponent(ComponentType::TRANSFORM));

    if (transform == nullptr) return; 

	// Global matrix 
    glm::mat4 modelMatrix = transform->GetGlobalMatrix();

	// Send matrix to shader
    GLuint shaderProgram = defaultShader->GetProgramID();
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    ComponentMaterial* material = static_cast<ComponentMaterial*>(gameObject->GetComponent(ComponentType::MATERIAL));

    // Use material texture or default texture
    if (material != nullptr && material->IsActive())
    {
        material->Use();
    }
    else
    {
        defaultTexture->Bind();
    }

	// Draw all mesh components
    std::vector<Component*> meshComponents = gameObject->GetComponentsOfType(ComponentType::MESH);

    for (Component* comp : meshComponents)
    {
        ComponentMesh* meshComp = static_cast<ComponentMesh*>(comp);

        if (meshComp->IsActive() && meshComp->HasMesh())
        {
            const Mesh& mesh = meshComp->GetMesh();
            DrawMesh(mesh); 
        }
    }

    // Unbind material or default texture
    if (material != nullptr && material->IsActive())
    {
        material->Unbind();
    }
    else
    {
        defaultTexture->Unbind();
    }

    // Recursively draw children
    const std::vector<GameObject*>& children = gameObject->GetChildren();
    for (GameObject* child : children)
    {
        DrawGameObjectRecursive(child);
    }
}
