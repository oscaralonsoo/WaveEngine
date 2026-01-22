#include "PhysicsDebugDrawer.h"
#include "Application.h"
#include <glad/glad.h>

PhysicsDebugDrawer::PhysicsDebugDrawer() : debugMode(0)
{
}

PhysicsDebugDrawer::~PhysicsDebugDrawer()
{
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
}

void PhysicsDebugDrawer::Init()
{
    // Generar buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Configuración de atributos: Posición (0) y Color (1)
    // Asumimos que el shader usa layout (location = 0) para pos y (location = 1) para color,
    // o un shader simple que solo usa pos. Adaptaremos según tu shader.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)sizeof(glm::vec3));

    glBindVertexArray(0);
}

void PhysicsDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    // Convertir de Bullet a GLM y añadir a la lista
    DebugVertex v1, v2;
    v1.position = glm::vec3(from.x(), from.y(), from.z());
    v1.color = glm::vec3(color.x(), color.y(), color.z());

    v2.position = glm::vec3(to.x(), to.y(), to.z());
    v2.color = glm::vec3(color.x(), color.y(), color.z());

    lines.push_back(v1);
    lines.push_back(v2);
}

void PhysicsDebugDrawer::Draw(const glm::mat4& view, const glm::mat4& projection)
{
    if (lines.empty()) return;

    // Obtener el shader de líneas del Renderer
    Shader* shader = Application::GetInstance().renderer->GetLineShader();
    if (!shader) return;

    shader->Use();
    shader->SetMat4("view", view);
    shader->SetMat4("projection", projection);
    shader->SetMat4("model", glm::mat4(1.0f)); // Identidad

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Subir datos nuevos cada frame (DYNAMIC_DRAW)
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(DebugVertex), lines.data(), GL_DYNAMIC_DRAW);

    // Dibujar líneas
    glDrawArrays(GL_LINES, 0, lines.size());

    glBindVertexArray(0);
    glUseProgram(0);

    // Limpiar para el siguiente frame
    lines.clear();
}

void PhysicsDebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
    // Opcional: dibujar una línea pequeña para el contacto
    btVector3 to = PointOnB + normalOnB * 1.0f; // 1 metro de normal
    drawLine(PointOnB, to, color);
}

void PhysicsDebugDrawer::reportErrorWarning(const char* warningString)
{
    LOG_CONSOLE("Bullet Warning: %s", warningString);
}

void PhysicsDebugDrawer::draw3dText(const btVector3& location, const char* textString)
{
    // Dejar vacío por ahora
}

void PhysicsDebugDrawer::setDebugMode(int debugMode)
{
    this->debugMode = debugMode; // <--- CORREGIDO: Usar this->debugMode
}

int PhysicsDebugDrawer::getDebugMode() const
{
    return debugMode; // <--- CORREGIDO: Usar debugMode
}