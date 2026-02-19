#pragma once
#include "Component.h"

class NavMeshManager;

class ComponentNavigation : public Component {
public:
    ComponentNavigation(GameObject* owner);

    void OnEditor() override;
    bool IsType(ComponentType type) override { return type == ComponentType::NAVIGATION; }
    bool IsIncompatible(ComponentType type) override { return false; }

    bool isStatic = true;
};