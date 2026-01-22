#pragma once

#include "Component.h"
#include <string>
#include <glm/glm.hpp>

class ReverbZone : public Component {
public:
    enum class Shape {
        SPHERE = 0,
        BOX = 1
    };

    ReverbZone(GameObject* owner);
    virtual ~ReverbZone();

    // point test in world space
    bool ContainsPoint(const glm::vec3& worldPoint) const;

    // Component overrides
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;
    void OnEditor() override;

public:
    Shape shape = Shape::SPHERE;
    float radius = 5.0f;                // sphere radius (world units)
    glm::vec3 extents = glm::vec3(5.0f); // box half-extents (local)
    std::string auxBusName = "Reverb_AuxBus"; // Wwise Aux Bus name
    float wetLevel = 1.0f;              // 0.0 - 1.0
    int priority = 0;                   // higher priority wins when overlapping
    bool enabled = true;
};