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
		hasExploded = true;

		// Trigger explosion - change particle system dramatically
		ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
			owner->GetComponent(ComponentType::PARTICLE_SYSTEM));

		if (ps)
		{
			// Reuse the tuned explosion preset instead of a custom setup
			ps->LoadExplosionPreset();
			ParticleSystemConfig& mainConfig = ps->GetMainConfig();
			mainConfig.startColor = explosionColor;
			ColorOverLifetimeModule& colorOverLife = ps->GetColorOverLifetime();
			colorOverLife.startColor = explosionColor;
			colorOverLife.endColor = glm::vec4(explosionColor.r, explosionColor.g, explosionColor.b, 0.0f);
			// Ensure the system restarts with the new preset parameters
			ps->Restart();
			LOG_CONSOLE("Firework exploded!");
		}
	}
    // Phase 3: Cleanup
    else if (currentTime > totalLifetime)
    {
        // Mark for deletion when particle system has finished
        ComponentParticleSystem* ps = static_cast<ComponentParticleSystem*>(
            owner->GetComponent(ComponentType::PARTICLE_SYSTEM));

        if (ps && !ps->IsAlive())
        {
            owner->MarkForDeletion();
            LOG_CONSOLE("Firework cleaned up");
        }
    }
}

void ComponentFirework::SetLaunchParameters(float speed, float expTime, float totalLife)
{
    upwardSpeed = speed;
    explosionTime = expTime;
    totalLifetime = totalLife;
}
