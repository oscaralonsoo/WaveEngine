#pragma once

#include "Component.h"
#include "AudioComponent.h"
#include <string>

class AudioSource : public Component, public AudioComponent {
public:
    AudioSource(GameObject* container);
    virtual ~AudioSource();

    //override from AudioComponent
    void SetTransform() override;

    ComponentType GetType() const override { return ComponentType::AUDIOSOURCE; }

    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    void OnEditor() override;

    ////Play sound by its name
    //void PlayEvent(const char* eventName);

    ////Play sound by its id
    //void PlayEvent(AkUniqueID eventID);

    //void StopEvent(const char* eventName);

public:
    std::string eventName = "";
    bool playOnAwake = false;
    bool hasAwoken = false;
    float volume = 100.0f;
};