#pragma once
#include "Module.h"
#include "Globals.h"
#include "Renderer.h"

#include <btBulletDynamicsCommon.h>
#include <vector>

class PhysicsDebugDrawer : public btIDebugDraw
{
public:
    PhysicsDebugDrawer();
    ~PhysicsDebugDrawer();

    // Inicializa los buffers (VAO/VBO)
    void Init();
    // Dibuja todas las líneas acumuladas y limpia el buffer
    void Draw(const glm::mat4& view, const glm::mat4& projection);

    // --- Métodos de la interfaz btIDebugDraw ---
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
    void reportErrorWarning(const char* warningString) override;
    void draw3dText(const btVector3& location, const char* textString) override;
    void setDebugMode(int debugMode) override;
    int getDebugMode() const override;

private:
    // Estructura simple para vértices de debug
    struct DebugVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<DebugVertex> lines;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int debugMode = 0;
};