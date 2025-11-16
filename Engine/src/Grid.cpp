#include "Grid.h"
#include <glad/glad.h>
#include <vector>
#include "Application.h"
#include "Shaders.h"
#include <glm/gtc/type_ptr.hpp>

Grid::Grid() : Module(), VAO(0), VBO(0), numVertices(0), gridSize(20.0f), gridDivisions(5), enabled(true)
{
}

Grid::~Grid()
{
}

bool Grid::Start()
{
    CreateGrid();
    return true;
}

void Grid::CreateGrid()
{
    std::vector<float> vertices;

    float halfSize = gridSize / 2.0f;
    float step = gridSize / gridDivisions;

    // Vertical lines (parallel to the Z axis)
    for (int i = 0; i <= gridDivisions; ++i)
    {
        float x = -halfSize + i * step;

        // Starting point
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(-halfSize);

        // Ending point
        vertices.push_back(x);
        vertices.push_back(0.0f);
        vertices.push_back(halfSize);
    }

    // Horizontal lines (parallel to the X axis)
    for (int i = 0; i <= gridDivisions; ++i)
    {
        float z = -halfSize + i * step;

        // Starting point
        vertices.push_back(-halfSize);
        vertices.push_back(0.0f);
        vertices.push_back(z);

        // Ending point
        vertices.push_back(halfSize);
        vertices.push_back(0.0f);
        vertices.push_back(z);
    }

    numVertices = vertices.size() / 3;

    // Generate VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

bool Grid::Update()
{
    if (enabled)
    {
        Draw();
    }
    return true;
}

void Grid::Draw()
{
    if (VAO == 0) return;

    Shader* shader = Application::GetInstance().renderer->GetDefaultShader();
    if (shader == nullptr) return;

    shader->Use();

    // Get camera matrices
    ComponentCamera* camera = Application::GetInstance().camera->GetActiveCamera();
    if (camera == nullptr) return;

    GLuint shaderProgram = shader->GetProgramID();

    // Send matrices to the shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

    // Identity model matrix (the grid is at the origin)
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

    // Configure to use solid color instead of texture
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glUniform4f(glGetUniformLocation(shaderProgram, "solidColor"), 0.5f, 0.5f, 0.5f, 1.0f); // Gray

    // Draw the grid
    glBindVertexArray(VAO);

    glDrawArrays(GL_LINES, 0, numVertices);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(0);
}

bool Grid::CleanUp()
{
    LOG_DEBUG("Cleaning up Grid");

    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    return true;
}
