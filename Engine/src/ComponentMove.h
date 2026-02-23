#pragma once
#include "Component.h"
#include <glm/glm.hpp>

class ComponentMove : public Component {
public:
    ComponentMove(GameObject* owner);
    ~ComponentMove() = default;

    void Update() override;
    void OnEditor() override;

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

public:
    float speed = 15.0f;
    float distance = 50.0f;
    glm::vec3 direction = { 1.0f, 0.0f, 0.0f }; 

private:
    float timer = 0.0f;
    bool movingForward = true;
};