#include "ComponentAudioListener.h"
#include "GameObject.h"
#include "Transform.h"

ComponentAudioListener::ComponentAudioListener(GameObject* owner)
    : Component(owner, ComponentType::AUDIO_LISTENER)
{
    name = "AudioListener";
    // ID estable: usamos la dirección del GameObject
    akId = (AkGameObjectID)(uintptr_t)owner;
}

void ComponentAudioListener::Enable()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio) return;

    // Registrar como listener en Wwise
    registered = audio->RegisterAudioGameObject(akId, "MainListener");

    if (registered)
    {
        // Establecer este GameObject como EL listener principal
        audio->SetListener(akId);
    }
}

void ComponentAudioListener::Update()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio || !registered) return;

    // Obtener el Transform del GameObject
    Component* c = owner->GetComponent(ComponentType::TRANSFORM);
    if (!c) return;

    // Cast al Transform real
    Transform* transform = static_cast<Transform*>(c);

    // Leer posición
    const glm::vec3& pos = transform->GetPosition();

    float px = pos.x;
    float py = pos.y;
    float pz = pos.z;

    // Actualizar posición del listener en Wwise
    audio->SetGameObjectTransform(akId,
        px, py, pz,
        0.0f, 0.0f, 1.0f,  // forward
        0.0f, 1.0f, 0.0f   // up
    );
}

void ComponentAudioListener::Disable()
{
    ModuleAudio* audio = ModuleAudio::Get();
    if (!audio) return;

    if (registered)
    {
        audio->UnregisterAudioGameObject(akId);
        registered = false;
    }
}