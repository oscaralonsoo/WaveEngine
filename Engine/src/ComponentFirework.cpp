#include "ComponentFirework.h"
#include "GameObject.h"
#include "Transform.h"
#include "ComponentParticleSystem.h"
#include "Application.h"
#include "Time.h"
#include "Log.h"

ComponentFirework::ComponentFirework(GameObject* owner)
    : Component(owner, ComponentType::FIREWORK),
    upwardSpeed(15.0f),
    explosionTime(1.5f),
    totalLifetime(8.0f),
    currentTime(0.0f),
    hasExploded(false),
    hasRequestedDeletion(false),
    explosionColor(1.0f, 1.0f, 1.0f, 1.0f)
{
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        startPosition = transform->GetPosition();
    }
}

ComponentFirework::~ComponentFirework()
{
}

void ComponentFirework::Update()
{
    if (!active) return;

    // CRITICAL FIX: Check deletion flag at the very start
    if (hasRequestedDeletion) return;

    Application::PlayState playState = Application::GetInstance().GetPlayState();
    if (playState == Application::PlayState::EDITING) return;

    if (!owner) return;

    float deltaTime = Application::GetInstance().time->GetDeltaTime();
    currentTime += deltaTime;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (!transform) return;

    // Phase 1: Ascent
    if (currentTime < explosionTime)
    {
        glm::vec3 currentPos = transform->GetPosition();
        currentPos.y = startPosition.y + upwardSpeed * currentTime;
        transform->SetPosition(currentPos);
    }
    // Phase 2: Explosion
    else if (!hasExploded)
    {
        TriggerExplosion();
    }
    // Phase 3: Cleanup
    else if (currentTime > totalLifetime)
    {
        // Check if particle system is done
        ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
            owner->GetComponent(ComponentType::PARTICLE_SYSTEM));

        if (ps && ps->IsAlive())
        {
            // Particles still alive, wait
            return;
        }

        // CRITICAL FIX: Set flag FIRST, mark for deletion LAST, then RETURN IMMEDIATELY
        if (!hasRequestedDeletion)
        {
            hasRequestedDeletion = true;
            LOG_CONSOLE("Firework cleaned up");
            owner->MarkForDeletion();

            // CRITICAL: Return immediately - do NOT access 'this' after this point
            return;
        }
    }
}

void ComponentFirework::TriggerExplosion()
{
    if (hasExploded) return;
    if (!owner) return;

    hasExploded = true;

    ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
        owner->GetComponent(ComponentType::PARTICLE_SYSTEM));

    if (!ps || !ps->IsActive()) return;

    try {
        ps->Stop();
        ps->LoadExplosionPreset();

        ParticleSystemConfig& mainConfig = ps->GetMainConfig();
        mainConfig.startColor = explosionColor;

        ColorOverLifetimeModule& colorOverLife = ps->GetColorOverLifetime();
        colorOverLife.startColor = explosionColor;
        colorOverLife.endColor = glm::vec4(explosionColor.r, explosionColor.g, explosionColor.b, 0.0f);

        ps->Restart();

        LOG_CONSOLE("Firework exploded!");
    }
    catch (...) {
        hasExploded = false;
    }
}

void ComponentFirework::OnEditor()
{
}

void ComponentFirework::SetLaunchParameters(float speed, float expTime, float totalLife)
{
    upwardSpeed = speed;
    explosionTime = expTime;
    totalLifetime = totalLife;
}

void ComponentFirework::Reset()
{
    if (!owner) return;

    currentTime = 0.0f;
    hasExploded = false;
    hasRequestedDeletion = false;

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform)
    {
        startPosition = transform->GetPosition();
    }

    ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
        owner->GetComponent(ComponentType::PARTICLE_SYSTEM));

    if (ps)
    {
        ps->Stop();
        ps->Clear();
    }
}