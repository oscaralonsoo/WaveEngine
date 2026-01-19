#include "ComponentAudioSource.h"
#include "GameObject.h"
#include "Transform.h"

ComponentAudioSource::ComponentAudioSource(GameObject* owner, unsigned int eventId, bool playOnEnable)
    : Component(owner, ComponentType::AUDIO_SOURCE), eventId(eventId), playOnEnable(playOnEnable)
{
    name = "AudioSource";
    // ID estable: usamos la dirección del GameObject
    akId = (AkGameObjectID)(uintptr_t)owner;
}

void ComponentAudioSource::Enable()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio) return;

    // Registrar el GameObject en Wwise
    registered = audio->RegisterAudioGameObject(akId, owner->GetName().c_str());

    postedOnce = false;

    // Si queremos que suene al activarse
    if (registered && playOnEnable && eventId != 0 && !postedOnce)
    {
        audio->PostEvent(eventId, akId);
        postedOnce = true;
    }
}

void ComponentAudioSource::Update()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio || !registered) return;

    // Obtener el Transform del GameObject
    Component* c = owner->GetComponent(ComponentType::TRANSFORM);
    if (!c) return;

    // Cast al Transform real
    Transform* transform = static_cast<Transform*>(c);

    // Leer posición (ahora funciona con glm::vec3)
    const glm::vec3& pos = transform->GetPosition();

    float px = pos.x;
    float py = pos.y;
    float pz = pos.z;

    // Actualizar posición en Wwise
    // Forward = (0, 0, 1), Up = (0, 1, 0)
    audio->SetGameObjectTransform(akId,
        px, py, pz,
        0.0f, 0.0f, 1.0f,  // forward
        0.0f, 1.0f, 0.0f   // up
    );
}

void ComponentAudioSource::Disable()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio) return;

    if (registered)
    {
        audio->UnregisterAudioGameObject(akId);
        registered = false;
    }
}