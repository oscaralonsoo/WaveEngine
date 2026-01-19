#pragma once
#include "Component.h"
#include "ModuleAudio.h"

class ComponentAudioListener : public Component
{
public:
    ComponentAudioListener(GameObject* owner);

    void Enable() override;
    void Update() override;
    void Disable() override;

private:
    AkGameObjectID akId = 0;
    bool registered = false;
};
