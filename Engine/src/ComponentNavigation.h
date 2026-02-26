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

};