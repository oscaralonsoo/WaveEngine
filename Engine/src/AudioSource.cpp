#include "AudioSource.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "AudioSystem.h"
#include "Log.h"
#include <glm/gtc/matrix_access.hpp> // Required for column extraction


AudioSource::AudioSource(GameObject* containerGO)
    : Component(containerGO, ComponentType::AUDIOSOURCE) // This sets the base owner
{
    //this->goID = (AkGameObjectID)this;
    this->goID = (AkGameObjectID)containerGO;
    this->GO = std::shared_ptr<GameObject>(containerGO, [](GameObject*) {});

    // Check if the base owner is valid before using it
    if (owner != nullptr) {
        AK::SoundEngine::RegisterGameObj(this->goID, owner->name.c_str());
        Application::GetInstance().audio->audioSystem->RegisterAudioComponent(this);
    }
}

AudioSource::~AudioSource()
{
    Application::GetInstance().audio->audioSystem->UnregisterAudioComponent(this);
    AK::SoundEngine::UnregisterGameObj(this->goID);
}

void AudioSource::SetTransform() {
    Transform* trans = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (trans) {
        // Use the recursive version to ensure we have the absolute truth
        glm::mat4 globalMat = trans->GetWorldMatrixRecursive();

        // Extract World Position (4th Column)
        glm::vec3 worldPos = glm::vec3(globalMat[3]);

        // Extract World Orientation (Rotation only)
        glm::vec3 worldForward = glm::normalize(glm::vec3(globalMat * glm::vec4(0, 0, 1, 0)));
        glm::vec3 worldUp = glm::normalize(glm::vec3(globalMat * glm::vec4(0, 1, 0, 0)));

        AkSoundPosition soundPos;
        soundPos.SetPosition(worldPos.x, worldPos.y, worldPos.z);

        soundPos.SetOrientation(
            worldForward.x, worldForward.y, worldForward.z,
            worldUp.x, worldUp.y, worldUp.z
        );

        AK::SoundEngine::SetRTPCValue("ObjectVolume", (AkRtpcValue)volume, goID);
        AK::SoundEngine::SetPosition(this->goID, soundPos);
    }
}

void AudioSource::Serialize(nlohmann::json& componentObj) const {
    //save event name
    componentObj["eventName"] = eventName;

    //save Play on Awake 
    componentObj["playOnAwake"] = playOnAwake;

    //save volume
    componentObj["volume"] = volume;
}

void AudioSource::Deserialize(const nlohmann::json& componentObj) {
    //load event name
    if (componentObj.contains("eventName")) {
        eventName = componentObj["eventName"].get<std::string>();
    }

    //load Play on Awake
    if (componentObj.contains("playOnAwake")) {
        playOnAwake = componentObj["playOnAwake"].get<bool>();
    }

    //load volume
    if (componentObj.contains("volume")) {
        volume = componentObj["volume"].get<float>();
    }

    //reset
    hasAwoken = false;
}

//void AudioSource::PlayEvent(char const* eventName)
//{
//    if (eventName == nullptr || strlen(eventName) == 0) return;
//    AK::SoundEngine::PostEvent(eventName, this->goID);
//}
//
//void AudioSource::PlayEvent(AkUniqueID eventID)
//{
//    AK::SoundEngine::PostEvent(eventID, this->goID);
//}
//
//void AudioSource::StopEvent(char const* eventName)
//{
//    if (eventName == nullptr || strlen(eventName) == 0) return;
//    AK::SoundEngine::ExecuteActionOnEvent(eventName, AK::SoundEngine::AkActionOnEventType_Stop, this->goID);
//}