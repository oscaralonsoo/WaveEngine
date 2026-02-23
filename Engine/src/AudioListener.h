#pragma once

#include "Component.h"
#include "AudioComponent.h"

class AudioListener : public Component, public AudioComponent {
public:
    AudioListener(GameObject* container);
    virtual ~AudioListener();

    // Override from AudioComponent
    void SetTransform() override;
    ComponentType GetType() const override { return ComponentType::LISTENER; }
    bool IsType(ComponentType type) override { return type == ComponentType::LISTENER; };
    bool IsIncompatible(ComponentType type) override { return false; };


    // Set as the default listener in the scene (like camara there must be a main one)
    void SetAsDefaultListener();

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void OnEditor() override;

private:
    bool isDefault = true;
};


