#pragma once
#include "Component.h"
#include "ModuleAudio.h"

class ComponentAudioSource : public Component
{
public:
    // eventId: por ejemplo AK::EVENTS::SFX_STATIC_PLAY
    ComponentAudioSource(GameObject* owner, unsigned int eventId, bool playOnEnable = true);

    void Enable() override;
    void Update() override;
    void Disable() override;

    void SetEvent(unsigned int newEventId) { eventId = newEventId; }
    unsigned int GetEvent() const { return eventId; }

private:
    AkGameObjectID akId = 0;
    unsigned int eventId = 0;
    bool playOnEnable = true;
    bool registered = false;
    bool postedOnce = false;
};
