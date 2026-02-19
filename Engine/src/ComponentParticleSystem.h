#pragma once
#include "Component.h"
#include "ParticleSystem.h"
#include <string>
#include <nlohmann/json.hpp>

class ComponentCamera;

class ComponentParticleSystem : public Component {
public:
    ComponentParticleSystem(GameObject* owner);
    virtual ~ComponentParticleSystem();

    void Update() override;
    void Draw(ComponentCamera* camera);

    bool IsType(ComponentType type) override { return type == ComponentType::PARTICLE; };
    bool IsIncompatible(ComponentType type) override { return false; };

    // Inspector UI for the module
    void OnEditor() override;

    // Serialization Save and Load Scene for play pause and stop
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Resource Management
    void SetTexture(const std::string& path);

    // Preset management for .particle files
    void SaveParticleFile(const std::string& path);
    void LoadParticleFile(const std::string& path);

private:
    EmitterInstance* emitter = nullptr;

    // Resource Reference Counting
    unsigned long long textureResourceUID = 0;

    std::string feedbackMessage = "";
    float feedbackTimer = 0.0f;
    bool feedbackIsError = false;
};