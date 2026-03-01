#pragma once
#include "Component.h"

class NavMeshManager;

enum class NavType
{
    SURFACE,
    AGENT,
    OBSTACLE
};

class ComponentNavigation : public Component {
public:
    ComponentNavigation(GameObject* owner);

    void OnEditor() override;
    bool IsType(ComponentType type) override { return type == ComponentType::NAVIGATION; }
    bool IsIncompatible(ComponentType type) override { return false; }

    bool isStatic = true;
    NavType type = NavType::SURFACE;

    float maxSlopeAngle = 35.0f;
    GameObject* linkedSurface = nullptr;

    // Parámetros
    float moveSpeed = 5.0f;
    float arrivalThreshold = 0.25f;

    // API pública
    bool SetDestination(const glm::vec3& worldTarget);
    void StopMovement();
    void Update(float dt);   // llámalo desde tu sistema de update
    bool IsMoving() const { return moving; }

    // Estado interno (privado si prefieres)
    std::vector<glm::vec3> path;
    int   pathIndex = 0;
    bool  moving = false;
private:
    bool SnapPositionToNavMesh(glm::vec3& position);

};