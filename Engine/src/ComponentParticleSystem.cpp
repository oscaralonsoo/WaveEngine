#include "ComponentParticleSystem.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "Time.h"
#include "Log.h"
#include "ModuleResources.h"
#include <random>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

ComponentParticleSystem::ComponentParticleSystem(GameObject* owner)
    : Component(owner, ComponentType::PARTICLE_SYSTEM),
    isPlaying(false), isPaused(false),
    systemTime(0.0f), accumulatedTime(0.0f),
    circleEmissionAngle(0.0f)
{
    name = "Particle System";
    particles.reserve(mainConfig.maxParticles);
    lastPosition = glm::vec3(0.0f);

    if (mainConfig.playOnAwake) {
        Play();
    }
}

ComponentParticleSystem::~ComponentParticleSystem() {
    if (mainConfig.textureUID != 0) {
        Application::GetInstance().resources->ReleaseResource(mainConfig.textureUID);
    }
}

void ComponentParticleSystem::Update() {
    if (!IsActive()) return;

    // IMPORTANTE: Actualizar SIEMPRE que isPlaying esté activo
    // Ya sea en modo editor o en modo play
    if (!isPlaying) return;

    if (isPaused) return;

    // Determinar el delta time apropiado
    float deltaTime;
    Application::PlayState playState = Application::GetInstance().GetPlayState();

    if (playState == Application::PlayState::EDITING) {
        // En modo editor, usar delta time real para preview suave
        deltaTime = Application::GetInstance().time->GetRealDeltaTime();

        // En editor, asegurar un mínimo para evitar frames muy largos
        if (deltaTime > 0.033f) deltaTime = 0.033f; // Cap a ~30 FPS
    }
    else {
        // En modo play, usar delta time del juego (respeta time scale)
        deltaTime = Application::GetInstance().time->GetDeltaTime();
    }

    systemTime += deltaTime;

    // Manejar fin de duración
    if (!mainConfig.looping && systemTime >= mainConfig.duration) {
        // Si no hace loop, detener la emisión pero mantener las partículas vivas
        if (GetActiveParticleCount() == 0) {
            Stop();
        }
        // No emitir más, pero seguir actualizando partículas existentes
        UpdateParticles(deltaTime);
        return;
    }

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 currentPosition = transform ? glm::vec3(transform->GetGlobalMatrix()[3]) : glm::vec3(0.0f);

    // EMISSION MODULE
    if (emissionModule.enabled) {
        // Rate over time
        if (emissionModule.rateOverTime > 0.0f) {
            accumulatedTime += deltaTime;
            float emitInterval = 1.0f / emissionModule.rateOverTime;

            while (accumulatedTime >= emitInterval) {
                EmitParticle();
                accumulatedTime -= emitInterval;
            }
        }

        // Rate over distance
        if (emissionModule.rateOverDistance > 0.0f) {
            float distance = glm::length(currentPosition - lastPosition);
            int particlesToEmit = static_cast<int>(distance * emissionModule.rateOverDistance);

            for (int i = 0; i < particlesToEmit; ++i) {
                EmitParticle();
            }
        }

        // Bursts
        for (size_t i = 0; i < emissionModule.bursts.size(); ++i) {
            if (i >= burstStates.size()) {
                BurstState newState;
                newState.burstIndex = (int)i;
                newState.cyclesCompleted = 0;
                newState.nextBurstTime = emissionModule.bursts[i].time;
                burstStates.push_back(newState);
            }

            BurstState& state = burstStates[i];
            const EmissionModule::Burst& burst = emissionModule.bursts[i];

            if (systemTime >= state.nextBurstTime && state.cyclesCompleted < burst.cycles) {
                for (int j = 0; j < burst.count; ++j) {
                    EmitParticle();
                }

                state.cyclesCompleted++;
                if (state.cyclesCompleted < burst.cycles) {
                    state.nextBurstTime += burst.repeatInterval;
                }
                else if (mainConfig.looping) {
                    state.cyclesCompleted = 0;
                    state.nextBurstTime = mainConfig.duration + burst.time;
                }
            }
        }
    }

    lastPosition = currentPosition;

    UpdateParticles(deltaTime);

    // Manejar loop
    if (mainConfig.looping && systemTime >= mainConfig.duration) {
        // Hacer loop del sistema
        float overflow = systemTime - mainConfig.duration;
        systemTime = overflow;

        // Resetear estados de burst
        for (auto& state : burstStates) {
            if (state.burstIndex >= 0 && state.burstIndex < (int)emissionModule.bursts.size()) {
                state.cyclesCompleted = 0;
                state.nextBurstTime = emissionModule.bursts[state.burstIndex].time;
            }
        }
    }
}

void ComponentParticleSystem::EmitParticle() {
    Particle* particle = nullptr;

    for (auto& p : particles) {
        if (!p.active) {
            particle = &p;
            break;
        }
    }

    if (!particle && particles.size() < static_cast<size_t>(mainConfig.maxParticles)) {
        particles.emplace_back();
        particle = &particles.back();
    }

    if (!particle) return;

    glm::vec3 emissionPos = GetEmissionPosition();
    glm::vec3 emissionDir = GetEmissionDirection();

    particle->active = true;
    particle->position = emissionPos;

    float speed = mainConfig.startSpeed + RandomRange(-mainConfig.startSpeedVariance, mainConfig.startSpeedVariance);
    particle->velocity = emissionDir * speed;

    particle->lifetime = 0.0f;
    particle->maxLifetime = mainConfig.startLifetime + RandomRange(-mainConfig.startLifetimeVariance, mainConfig.startLifetimeVariance);
    if (particle->maxLifetime < 0.1f) particle->maxLifetime = 0.1f;

    particle->size = mainConfig.startSize + RandomRange(-mainConfig.startSizeVariance, mainConfig.startSizeVariance);
    if (particle->size < 0.01f) particle->size = 0.01f;

    particle->rotation = mainConfig.startRotation + RandomRange(-mainConfig.startRotationVariance, mainConfig.startRotationVariance);
    particle->rotationSpeed = 0.0f;

    particle->color = mainConfig.startColor;
}

void ComponentParticleSystem::UpdateParticles(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.active) continue;

        particle.lifetime += deltaTime;

        if (particle.lifetime >= particle.maxLifetime) {
            particle.active = false;
            continue;
        }

        float normalizedLifetime = particle.lifetime / particle.maxLifetime;

        if (mainConfig.gravityModifier != 0.0f) {
            particle.velocity.y += -9.81f * mainConfig.gravityModifier * deltaTime;
        }

        if (velocityOverLifetime.enabled) {
            ApplyVelocityOverLifetime(particle, normalizedLifetime);
        }

        if (limitVelocityOverLifetime.enabled) {
            ApplyLimitVelocityOverLifetime(particle);
        }

        if (forceOverLifetime.enabled) {
            ApplyForceOverLifetime(particle, deltaTime, normalizedLifetime);
        }

        if (noiseModule.enabled) {
            ApplyNoise(particle, deltaTime);
        }

        particle.position += particle.velocity * deltaTime;

        if (colorOverLifetime.enabled) {
            ApplyColorOverLifetime(particle, normalizedLifetime);
        }

        if (colorBySpeed.enabled) {
            ApplyColorBySpeed(particle);
        }

        if (sizeOverLifetime.enabled) {
            ApplySizeOverLifetime(particle, normalizedLifetime);
        }

        if (sizeBySpeed.enabled) {
            ApplySizeBySpeed(particle);
        }

        if (rotationOverLifetime.enabled) {
            ApplyRotationOverLifetime(particle, deltaTime, normalizedLifetime);
        }

        if (rotationBySpeed.enabled) {
            ApplyRotationBySpeed(particle);
        }

        particle.rotation += particle.rotationSpeed * deltaTime;
    }
}

// MODULOS

void ComponentParticleSystem::ApplyVelocityOverLifetime(Particle& particle, float normalizedLifetime) {
    glm::vec3 velocity = velocityOverLifetime.velocity * normalizedLifetime;

    if (velocityOverLifetime.isLocal) {
        Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        if (transform) {
            glm::mat4 modelMatrix = transform->GetGlobalMatrix();
            glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
            velocity = rotationMatrix * velocity;
        }
    }

    particle.velocity += velocity;
}

void ComponentParticleSystem::ApplyLimitVelocityOverLifetime(Particle& particle) {
    float currentSpeed = glm::length(particle.velocity);

    if (currentSpeed > limitVelocityOverLifetime.speed) {
        float reduction = 1.0f - limitVelocityOverLifetime.dampen;
        particle.velocity *= reduction;
    }
}

void ComponentParticleSystem::ApplyForceOverLifetime(Particle& particle, float deltaTime, float normalizedLifetime) {
    glm::vec3 force = forceOverLifetime.force;

    if (forceOverLifetime.isLocal) {
        Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        if (transform) {
            glm::mat4 modelMatrix = transform->GetGlobalMatrix();
            glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
            force = rotationMatrix * force;
        }
    }

    particle.velocity += force * deltaTime;
}

void ComponentParticleSystem::ApplyColorOverLifetime(Particle& particle, float normalizedLifetime) {
    if (!colorOverLifetime.gradient.empty()) {
        particle.color = EvaluateGradient(colorOverLifetime.gradient, normalizedLifetime);
    }
    else {
        particle.color = glm::mix(colorOverLifetime.startColor, colorOverLifetime.endColor, normalizedLifetime);
    }
}

void ComponentParticleSystem::ApplyColorBySpeed(Particle& particle) {
    float speed = glm::length(particle.velocity);
    float t = glm::clamp((speed - colorBySpeed.minSpeed) / (colorBySpeed.maxSpeed - colorBySpeed.minSpeed), 0.0f, 1.0f);
    glm::vec4 speedColor = glm::mix(colorBySpeed.minColor, colorBySpeed.maxColor, t);
    particle.color *= speedColor;
}

void ComponentParticleSystem::ApplySizeOverLifetime(Particle& particle, float normalizedLifetime) {
    float sizeMultiplier;

    if (!sizeOverLifetime.sizeCurve.empty()) {
        sizeMultiplier = EvaluateCurve(sizeOverLifetime.sizeCurve, normalizedLifetime);
    }
    else {
        sizeMultiplier = glm::mix(sizeOverLifetime.startSize, sizeOverLifetime.endSize, normalizedLifetime);
    }

    particle.size = mainConfig.startSize * sizeMultiplier;
}

void ComponentParticleSystem::ApplySizeBySpeed(Particle& particle) {
    float speed = glm::length(particle.velocity);
    float t = glm::clamp((speed - sizeBySpeed.minSpeed) / (sizeBySpeed.maxSpeed - sizeBySpeed.minSpeed), 0.0f, 1.0f);
    float speedSize = glm::mix(sizeBySpeed.minSize, sizeBySpeed.maxSize, t);
    particle.size *= speedSize;
}

void ComponentParticleSystem::ApplyRotationOverLifetime(Particle& particle, float deltaTime, float normalizedLifetime) {
    float angularVel = rotationOverLifetime.angularVelocity;
    angularVel += RandomRange(-rotationOverLifetime.angularVelocityVariance, rotationOverLifetime.angularVelocityVariance);
    particle.rotationSpeed = angularVel;
}

void ComponentParticleSystem::ApplyRotationBySpeed(Particle& particle) {
    float speed = glm::length(particle.velocity);
    float t = glm::clamp((speed - rotationBySpeed.minSpeed) / (rotationBySpeed.maxSpeed - rotationBySpeed.minSpeed), 0.0f, 1.0f);
    float speedRotation = glm::mix(rotationBySpeed.minAngularVelocity, rotationBySpeed.maxAngularVelocity, t);
    particle.rotationSpeed = speedRotation;
}

void ComponentParticleSystem::ApplyNoise(Particle& particle, float deltaTime) {
    float time = systemTime * noiseModule.scrollSpeed;
    float noiseX = sin((particle.position.x + time) * noiseModule.frequency) * noiseModule.strength;
    float noiseY = cos((particle.position.y + time) * noiseModule.frequency) * noiseModule.strength;
    float noiseZ = sin((particle.position.z + time) * noiseModule.frequency) * noiseModule.strength;

    glm::vec3 noiseForce(noiseX, noiseY, noiseZ);
    particle.velocity += noiseForce * deltaTime;

    if (noiseModule.damping) {
        particle.velocity *= 0.99f;
    }
}

// EMISSION SHAPES

glm::vec3 ComponentParticleSystem::GetEmissionPosition() {
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 emitterPos = transform ? glm::vec3(transform->GetGlobalMatrix()[3]) : glm::vec3(0.0f);

    if (!shapeModule.enabled) {
        return emitterPos;
    }

    glm::vec3 localPos;

    switch (shapeModule.shape) {
    case EmitterShape::CONE:
        localPos = GetConeEmissionPosition();
        break;
    case EmitterShape::SPHERE:
    case EmitterShape::HEMISPHERE:
        localPos = GetSphereEmissionPosition();
        break;
    case EmitterShape::CIRCLE:
        localPos = GetCircleEmissionPosition();
        break;
    case EmitterShape::BOX:
        localPos = GetBoxEmissionPosition();
        break;
    case EmitterShape::EDGE:
        localPos = GetEdgeEmissionPosition();
        break;
    default:
        localPos = glm::vec3(0.0f);
    }

    if (transform && mainConfig.simulationSpace == ParticleSystemConfig::SimulationSpace::LOCAL) {
        glm::mat4 modelMatrix = transform->GetGlobalMatrix();
        return glm::vec3(modelMatrix * glm::vec4(localPos, 1.0f));
    }

    return emitterPos + localPos;
}

glm::vec3 ComponentParticleSystem::GetEmissionDirection() {
    if (!shapeModule.enabled) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 direction;

    switch (shapeModule.shape) {
    case EmitterShape::CONE:
        direction = GetConeEmissionDirection();
        break;
    case EmitterShape::SPHERE:
    case EmitterShape::HEMISPHERE:
        direction = GetSphereEmissionDirection();
        break;
    case EmitterShape::CIRCLE:
        direction = GetCircleEmissionDirection();
        break;
    case EmitterShape::BOX:
        direction = GetBoxEmissionDirection();
        break;
    case EmitterShape::EDGE:
        direction = GetEdgeEmissionDirection();
        break;
    default:
        direction = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (shapeModule.randomDirectionAmount > 0.0f) {
        glm::vec3 randomDir = RandomOnUnitSphere();
        direction = glm::mix(direction, randomDir, shapeModule.randomDirectionAmount);
    }

    if (shapeModule.sphericalDirectionAmount > 0.0f) {
        Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
        glm::vec3 emitterPos = transform ? glm::vec3(transform->GetGlobalMatrix()[3]) : glm::vec3(0.0f);
        glm::vec3 particlePos = GetEmissionPosition();
        glm::vec3 sphericalDir = glm::normalize(particlePos - emitterPos);
        direction = glm::mix(direction, sphericalDir, shapeModule.sphericalDirectionAmount);
    }

    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    if (transform && mainConfig.simulationSpace == ParticleSystemConfig::SimulationSpace::LOCAL) {
        glm::mat4 modelMatrix = transform->GetGlobalMatrix();
        glm::mat3 rotationMatrix = glm::mat3(modelMatrix);
        direction = rotationMatrix * direction;
    }

    return glm::normalize(direction);
}

glm::vec3 ComponentParticleSystem::GetConeEmissionPosition() {
    if (shapeModule.emitFromBase) {
        float angle = RandomRange(0.0f, glm::two_pi<float>());
        float radius = RandomRange(0.0f, shapeModule.coneRadius);
        return glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
    }
    else {
        float t = RandomRange(0.0f, 1.0f);
        float angle = RandomRange(0.0f, glm::two_pi<float>());
        float radius = shapeModule.coneRadius * t;
        float height = shapeModule.coneLength * t;
        return glm::vec3(cos(angle) * radius, height, sin(angle) * radius);
    }
}

glm::vec3 ComponentParticleSystem::GetConeEmissionDirection() {
    float coneAngleRad = glm::radians(shapeModule.coneAngle);
    float randomAngle = RandomRange(-coneAngleRad, coneAngleRad);
    float phi = RandomRange(0.0f, glm::two_pi<float>());

    glm::vec3 direction;
    direction.x = sin(randomAngle) * cos(phi);
    direction.y = cos(randomAngle);
    direction.z = sin(randomAngle) * sin(phi);

    return glm::normalize(direction);
}

glm::vec3 ComponentParticleSystem::GetSphereEmissionPosition() {
    float radius = shapeModule.sphereRadius;

    if (shapeModule.sphereRadiusThickness < 1.0f) {
        float minRadius = radius * shapeModule.sphereRadiusThickness;
        radius = RandomRange(minRadius, radius);
    }
    else {
        radius *= pow(RandomRange(0.0f, 1.0f), 1.0f / 3.0f);
    }

    glm::vec3 point = RandomOnUnitSphere() * radius;

    if (shapeModule.shape == EmitterShape::HEMISPHERE && point.y < 0.0f) {
        point.y = -point.y;
    }

    return point;
}

glm::vec3 ComponentParticleSystem::GetSphereEmissionDirection() {
    return glm::normalize(GetSphereEmissionPosition());
}

glm::vec3 ComponentParticleSystem::GetCircleEmissionPosition() {
    float angle = 0.0f;

    switch (shapeModule.circleMode) {
    case EmissionMode::RANDOM:
        angle = RandomRange(0.0f, glm::radians(shapeModule.circleArc));
        break;
    case EmissionMode::LOOP:
        circleEmissionAngle += shapeModule.circleSpeed;
        angle = fmod(circleEmissionAngle, glm::radians(shapeModule.circleArc));
        break;
    case EmissionMode::PING_PONG:
        angle = RandomRange(0.0f, glm::radians(shapeModule.circleArc));
        break;
    }

    float radius = shapeModule.circleRadius;
    float spread = RandomRange(-shapeModule.circleSpread, shapeModule.circleSpread);

    return glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius + spread);
}

glm::vec3 ComponentParticleSystem::GetCircleEmissionDirection() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 ComponentParticleSystem::GetBoxEmissionPosition() {
    glm::vec3 halfSize = shapeModule.boxSize * 0.5f;

    if (shapeModule.emitFromVolume) {
        return glm::vec3(
            RandomRange(-halfSize.x, halfSize.x),
            RandomRange(-halfSize.y, halfSize.y),
            RandomRange(-halfSize.z, halfSize.z)
        );
    }
    else {
        int face = rand() % 6;
        glm::vec3 pos;

        switch (face) {
        case 0: pos = glm::vec3(halfSize.x, RandomRange(-halfSize.y, halfSize.y), RandomRange(-halfSize.z, halfSize.z)); break;
        case 1: pos = glm::vec3(-halfSize.x, RandomRange(-halfSize.y, halfSize.y), RandomRange(-halfSize.z, halfSize.z)); break;
        case 2: pos = glm::vec3(RandomRange(-halfSize.x, halfSize.x), halfSize.y, RandomRange(-halfSize.z, halfSize.z)); break;
        case 3: pos = glm::vec3(RandomRange(-halfSize.x, halfSize.x), -halfSize.y, RandomRange(-halfSize.z, halfSize.z)); break;
        case 4: pos = glm::vec3(RandomRange(-halfSize.x, halfSize.x), RandomRange(-halfSize.y, halfSize.y), halfSize.z); break;
        case 5: pos = glm::vec3(RandomRange(-halfSize.x, halfSize.x), RandomRange(-halfSize.y, halfSize.y), -halfSize.z); break;
        }

        return pos;
    }
}

glm::vec3 ComponentParticleSystem::GetBoxEmissionDirection() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 ComponentParticleSystem::GetEdgeEmissionPosition() {
    float angle = RandomRange(0.0f, glm::two_pi<float>());
    float radius = shapeModule.edgeRadius;
    return glm::vec3(cos(angle) * radius, 0.0f, sin(angle) * radius);
}

glm::vec3 ComponentParticleSystem::GetEdgeEmissionDirection() {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

// UTILITIES

float ComponentParticleSystem::RandomRange(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

glm::vec3 ComponentParticleSystem::RandomInUnitSphere() {
    glm::vec3 point;
    do {
        point = glm::vec3(
            RandomRange(-1.0f, 1.0f),
            RandomRange(-1.0f, 1.0f),
            RandomRange(-1.0f, 1.0f)
        );
    } while (glm::length(point) > 1.0f);
    return point;
}

glm::vec3 ComponentParticleSystem::RandomOnUnitSphere() {
    return glm::normalize(RandomInUnitSphere());
}

float ComponentParticleSystem::EvaluateCurve(const std::vector<SizeOverLifetimeModule::SizeKey>& curve, float time) {
    if (curve.empty()) return 1.0f;
    if (curve.size() == 1) return curve[0].size;

    for (size_t i = 0; i < curve.size() - 1; ++i) {
        if (time >= curve[i].time && time <= curve[i + 1].time) {
            float t = (time - curve[i].time) / (curve[i + 1].time - curve[i].time);
            return glm::mix(curve[i].size, curve[i + 1].size, t);
        }
    }

    return curve.back().size;
}

glm::vec4 ComponentParticleSystem::EvaluateGradient(const std::vector<ColorOverLifetimeModule::ColorKey>& gradient, float time) {
    if (gradient.empty()) return glm::vec4(1.0f);
    if (gradient.size() == 1) return gradient[0].color;

    for (size_t i = 0; i < gradient.size() - 1; ++i) {
        if (time >= gradient[i].time && time <= gradient[i + 1].time) {
            float t = (time - gradient[i].time) / (gradient[i + 1].time - gradient[i].time);
            return glm::mix(gradient[i].color, gradient[i + 1].color, t);
        }
    }

    return gradient.back().color;
}

// CONTROL

void ComponentParticleSystem::Play() {
    isPlaying = true;
    isPaused = false;

    if (mainConfig.prewarm && mainConfig.looping) {
        float simulationTime = 0.0f;
        float step = 0.1f;

        while (simulationTime < mainConfig.duration) {
            float oldSystemTime = systemTime;
            systemTime = simulationTime;

            Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
            glm::vec3 currentPosition = transform ? glm::vec3(transform->GetGlobalMatrix()[3]) : glm::vec3(0.0f);

            if (emissionModule.enabled && emissionModule.rateOverTime > 0.0f) {
                accumulatedTime += step;
                float emitInterval = 1.0f / emissionModule.rateOverTime;

                while (accumulatedTime >= emitInterval) {
                    EmitParticle();
                    accumulatedTime -= emitInterval;
                }
            }

            UpdateParticles(step);

            simulationTime += step;
        }

        systemTime = 0.0f; 
    }
}

void ComponentParticleSystem::Stop() {
    isPlaying = false;
    isPaused = false;
    Clear();
}

void ComponentParticleSystem::Pause() {
    isPaused = !isPaused;
}

void ComponentParticleSystem::Restart() {
    Stop();
    systemTime = 0.0f;
    accumulatedTime = 0.0f;
    burstStates.clear();
    circleEmissionAngle = 0.0f;
    Play();
}

void ComponentParticleSystem::Clear() {
    for (auto& p : particles) {
        p.active = false;
    }
    systemTime = 0.0f;
    accumulatedTime = 0.0f;
    burstStates.clear();
}

bool ComponentParticleSystem::IsAlive() const {
    if (isPlaying && !isPaused) return true;

    for (const auto& p : particles) {
        if (p.active) return true;
    }

    return false;
}

int ComponentParticleSystem::GetActiveParticleCount() const {
    int count = 0;
    for (const auto& p : particles) {
        if (p.active) count++;
    }
    return count;
}

bool ComponentParticleSystem::LoadTexture(const std::string& path) {
    ModuleResources* resources = Application::GetInstance().resources.get();
    if (!resources) return false;

    // Release previous texture
    if (mainConfig.textureUID != 0) {
        resources->ReleaseResource(mainConfig.textureUID);
        mainConfig.textureUID = 0;
    }

    // Find or import the texture
    UID uid = resources->Find(path.c_str());
    if (uid == 0) {
        uid = resources->ImportFile(path.c_str());
    }

    if (uid == 0) {
        LOG_DEBUG("Failed to load particle texture: %s", path.c_str());
        return false;
    }

    // Get the resource and ensure it's loaded
    Resource* resource = resources->GetResourceDirect(uid);
    if (!resource) {
        LOG_DEBUG("Failed to get particle texture resource");
        return false;
    }

    // Load to memory if not already loaded
    if (!resource->IsLoadedToMemory()) {
        if (resources->RequestResource(uid) == 0) {
            LOG_DEBUG("Failed to load particle texture to memory");
            return false;
        }
    }

    mainConfig.textureUID = uid;
    mainConfig.texturePath = path;

    LOG_DEBUG("Particle texture loaded successfully: %s (UID: %llu)", path.c_str(), uid);
    return true;
}

void ComponentParticleSystem::OnEditor() {
    // empty
}

// PRESETS

void ComponentParticleSystem::LoadSmokePreset() {
    // Main
    mainConfig.startLifetime = 3.0f;
    mainConfig.startSpeed = 1.0f;
    mainConfig.startSize = 1.0f;
    mainConfig.gravityModifier = -0.2f;

    // Emission
    emissionModule.enabled = true;
    emissionModule.rateOverTime = 10.0f;

    // Shape
    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::CONE;
    shapeModule.coneAngle = 20.0f;

    // Color
    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(0.7f, 0.7f, 0.7f, 0.8f);
    colorOverLifetime.endColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.0f);

    // Size
    sizeOverLifetime.enabled = true;
    sizeOverLifetime.startSize = 1.0f;
    sizeOverLifetime.endSize = 2.0f;

    // Rotation
    rotationOverLifetime.enabled = true;
    rotationOverLifetime.angularVelocity = 20.0f;
    rotationOverLifetime.angularVelocityVariance = 10.0f;
}

void ComponentParticleSystem::LoadFirePreset() {
    mainConfig.startLifetime = 1.5f;
    mainConfig.startSpeed = 2.0f;
    mainConfig.startSize = 0.5f;
    mainConfig.gravityModifier = -0.5f;

    emissionModule.enabled = true;
    emissionModule.rateOverTime = 20.0f;

    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::CONE;
    shapeModule.coneAngle = 15.0f;

    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    colorOverLifetime.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

    sizeOverLifetime.enabled = true;
    sizeOverLifetime.startSize = 1.0f;
    sizeOverLifetime.endSize = 0.5f;

    rotationOverLifetime.enabled = true;
    rotationOverLifetime.angularVelocity = 50.0f;
    rotationOverLifetime.angularVelocityVariance = 30.0f;
}

void ComponentParticleSystem::LoadExplosionPreset() {
    mainConfig.startLifetime = 2.0f;
    mainConfig.startSpeed = 5.0f;
    mainConfig.startSize = 0.3f;
    mainConfig.gravityModifier = 1.0f;
    mainConfig.looping = false;
    mainConfig.duration = 0.5f;

    emissionModule.enabled = true;
    emissionModule.rateOverTime = 0.0f;

    EmissionModule::Burst burst;
    burst.time = 0.0f;
    burst.count = 200;
    burst.cycles = 1;
    emissionModule.bursts.clear();
    emissionModule.bursts.push_back(burst);

    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::SPHERE;
    shapeModule.sphereRadius = 0.1f;

    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f);
    colorOverLifetime.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);

    sizeOverLifetime.enabled = true;
    sizeOverLifetime.startSize = 1.0f;
    sizeOverLifetime.endSize = 0.2f;
}

void ComponentParticleSystem::LoadSparklesPreset() {
    mainConfig.startLifetime = 2.0f;
    mainConfig.startSpeed = 1.0f;
    mainConfig.startSize = 0.2f;
    mainConfig.gravityModifier = 0.5f;

    emissionModule.enabled = true;
    emissionModule.rateOverTime = 30.0f;

    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::SPHERE;
    shapeModule.sphereRadius = 0.5f;

    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    colorOverLifetime.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

    sizeOverLifetime.enabled = true;
    sizeOverLifetime.startSize = 1.0f;
    sizeOverLifetime.endSize = 0.5f;

    rotationOverLifetime.enabled = true;
    rotationOverLifetime.angularVelocity = 200.0f;
    rotationOverLifetime.angularVelocityVariance = 100.0f;
}

void ComponentParticleSystem::LoadRainPreset() {
    mainConfig.startLifetime = 2.0f;
    mainConfig.startSpeed = 10.0f;
    mainConfig.startSize = 0.1f;
    mainConfig.gravityModifier = 2.0f;

    emissionModule.enabled = true;
    emissionModule.rateOverTime = 100.0f;

    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::BOX;
    shapeModule.boxSize = glm::vec3(10.0f, 0.1f, 10.0f);
    shapeModule.emitFromVolume = true;

    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(0.5f, 0.7f, 1.0f, 0.8f);
    colorOverLifetime.endColor = glm::vec4(0.5f, 0.7f, 1.0f, 0.4f);

    sizeOverLifetime.enabled = false;
}

void ComponentParticleSystem::LoadSnowPreset() {
    mainConfig.startLifetime = 5.0f;
    mainConfig.startSpeed = 0.5f;
    mainConfig.startSize = 0.2f;
    mainConfig.gravityModifier = 0.1f;

    emissionModule.enabled = true;
    emissionModule.rateOverTime = 50.0f;

    shapeModule.enabled = true;
    shapeModule.shape = EmitterShape::BOX;
    shapeModule.boxSize = glm::vec3(10.0f, 0.1f, 10.0f);
    shapeModule.emitFromVolume = true;

    colorOverLifetime.enabled = true;
    colorOverLifetime.startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    colorOverLifetime.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);

    noiseModule.enabled = true;
    noiseModule.strength = 0.5f;
    noiseModule.frequency = 0.3f;

    rotationOverLifetime.enabled = true;
    rotationOverLifetime.angularVelocity = 30.0f;
    rotationOverLifetime.angularVelocityVariance = 20.0f;
}

// SERIALIZATION

void ComponentParticleSystem::Serialize(nlohmann::json& componentObj) const {
    // Main config
    componentObj["duration"] = mainConfig.duration;
    componentObj["looping"] = mainConfig.looping;
    componentObj["prewarm"] = mainConfig.prewarm;
    componentObj["startDelay"] = mainConfig.startDelay;
    componentObj["startLifetime"] = mainConfig.startLifetime;
    componentObj["startLifetimeVariance"] = mainConfig.startLifetimeVariance;
    componentObj["startSpeed"] = mainConfig.startSpeed;
    componentObj["startSpeedVariance"] = mainConfig.startSpeedVariance;
    componentObj["startSize"] = mainConfig.startSize;
    componentObj["startSizeVariance"] = mainConfig.startSizeVariance;
    componentObj["startRotation"] = mainConfig.startRotation;
    componentObj["startRotationVariance"] = mainConfig.startRotationVariance;
    componentObj["startColor"] = { mainConfig.startColor.r, mainConfig.startColor.g, mainConfig.startColor.b, mainConfig.startColor.a };
    componentObj["gravityModifier"] = mainConfig.gravityModifier;
    componentObj["simulationSpace"] = (int)mainConfig.simulationSpace;
    componentObj["maxParticles"] = mainConfig.maxParticles;
    componentObj["renderMode"] = (int)mainConfig.renderMode;
    componentObj["blendMode"] = (int)mainConfig.blendMode;
    componentObj["textureUID"] = mainConfig.textureUID;
    componentObj["texturePath"] = mainConfig.texturePath;

    // Emission module
    componentObj["emission_enabled"] = emissionModule.enabled;
    componentObj["emission_rateOverTime"] = emissionModule.rateOverTime;
    componentObj["emission_rateOverDistance"] = emissionModule.rateOverDistance;

    // Shape module
    componentObj["shape_enabled"] = shapeModule.enabled;
    componentObj["shape_type"] = (int)shapeModule.shape;
    componentObj["shape_coneAngle"] = shapeModule.coneAngle;
    componentObj["shape_coneRadius"] = shapeModule.coneRadius;
    componentObj["shape_sphereRadius"] = shapeModule.sphereRadius;

    // Color over lifetime
    componentObj["colorOverLifetime_enabled"] = colorOverLifetime.enabled;
    componentObj["colorOverLifetime_startColor"] = { colorOverLifetime.startColor.r, colorOverLifetime.startColor.g, colorOverLifetime.startColor.b, colorOverLifetime.startColor.a };
    componentObj["colorOverLifetime_endColor"] = { colorOverLifetime.endColor.r, colorOverLifetime.endColor.g, colorOverLifetime.endColor.b, colorOverLifetime.endColor.a };

    // Size over lifetime
    componentObj["sizeOverLifetime_enabled"] = sizeOverLifetime.enabled;
    componentObj["sizeOverLifetime_startSize"] = sizeOverLifetime.startSize;
    componentObj["sizeOverLifetime_endSize"] = sizeOverLifetime.endSize;

    // State
    componentObj["isPlaying"] = isPlaying;
}

void ComponentParticleSystem::Deserialize(const nlohmann::json& componentObj) {
    // Main config
    if (componentObj.contains("duration")) mainConfig.duration = componentObj["duration"];
    if (componentObj.contains("looping")) mainConfig.looping = componentObj["looping"];
    if (componentObj.contains("prewarm")) mainConfig.prewarm = componentObj["prewarm"];
    if (componentObj.contains("startDelay")) mainConfig.startDelay = componentObj["startDelay"];
    if (componentObj.contains("startLifetime")) mainConfig.startLifetime = componentObj["startLifetime"];
    if (componentObj.contains("startLifetimeVariance")) mainConfig.startLifetimeVariance = componentObj["startLifetimeVariance"];
    if (componentObj.contains("startSpeed")) mainConfig.startSpeed = componentObj["startSpeed"];
    if (componentObj.contains("startSpeedVariance")) mainConfig.startSpeedVariance = componentObj["startSpeedVariance"];
    if (componentObj.contains("startSize")) mainConfig.startSize = componentObj["startSize"];
    if (componentObj.contains("startSizeVariance")) mainConfig.startSizeVariance = componentObj["startSizeVariance"];
    if (componentObj.contains("startRotation")) mainConfig.startRotation = componentObj["startRotation"];
    if (componentObj.contains("startRotationVariance")) mainConfig.startRotationVariance = componentObj["startRotationVariance"];

    if (componentObj.contains("startColor")) {
        auto c = componentObj["startColor"];
        mainConfig.startColor = glm::vec4(c[0], c[1], c[2], c[3]);
    }

    if (componentObj.contains("gravityModifier")) mainConfig.gravityModifier = componentObj["gravityModifier"];
    if (componentObj.contains("simulationSpace")) mainConfig.simulationSpace = (ParticleSystemConfig::SimulationSpace)componentObj["simulationSpace"].get<int>();
    if (componentObj.contains("maxParticles")) mainConfig.maxParticles = componentObj["maxParticles"];
    if (componentObj.contains("renderMode")) mainConfig.renderMode = (ParticleSystemConfig::RenderMode)componentObj["renderMode"].get<int>();
    if (componentObj.contains("blendMode")) mainConfig.blendMode = (ParticleSystemConfig::BlendMode)componentObj["blendMode"].get<int>();
    if (componentObj.contains("textureUID")) mainConfig.textureUID = componentObj["textureUID"];
    if (componentObj.contains("texturePath")) mainConfig.texturePath = componentObj["texturePath"];

    // Emission module
    if (componentObj.contains("emission_enabled")) emissionModule.enabled = componentObj["emission_enabled"];
    if (componentObj.contains("emission_rateOverTime")) emissionModule.rateOverTime = componentObj["emission_rateOverTime"];
    if (componentObj.contains("emission_rateOverDistance")) emissionModule.rateOverDistance = componentObj["emission_rateOverDistance"];

    // Shape module
    if (componentObj.contains("shape_enabled")) shapeModule.enabled = componentObj["shape_enabled"];
    if (componentObj.contains("shape_type")) shapeModule.shape = (EmitterShape)componentObj["shape_type"].get<int>();
    if (componentObj.contains("shape_coneAngle")) shapeModule.coneAngle = componentObj["shape_coneAngle"];
    if (componentObj.contains("shape_coneRadius")) shapeModule.coneRadius = componentObj["shape_coneRadius"];
    if (componentObj.contains("shape_sphereRadius")) shapeModule.sphereRadius = componentObj["shape_sphereRadius"];

    // Color over lifetime
    if (componentObj.contains("colorOverLifetime_enabled")) colorOverLifetime.enabled = componentObj["colorOverLifetime_enabled"];
    if (componentObj.contains("colorOverLifetime_startColor")) {
        auto c = componentObj["colorOverLifetime_startColor"];
        colorOverLifetime.startColor = glm::vec4(c[0], c[1], c[2], c[3]);
    }
    if (componentObj.contains("colorOverLifetime_endColor")) {
        auto c = componentObj["colorOverLifetime_endColor"];
        colorOverLifetime.endColor = glm::vec4(c[0], c[1], c[2], c[3]);
    }

    // Size over lifetime
    if (componentObj.contains("sizeOverLifetime_enabled")) sizeOverLifetime.enabled = componentObj["sizeOverLifetime_enabled"];
    if (componentObj.contains("sizeOverLifetime_startSize")) sizeOverLifetime.startSize = componentObj["sizeOverLifetime_startSize"];
    if (componentObj.contains("sizeOverLifetime_endSize")) sizeOverLifetime.endSize = componentObj["sizeOverLifetime_endSize"];

    // State
    if (componentObj.contains("isPlaying")) {
        bool wasPlaying = componentObj["isPlaying"];
        if (wasPlaying) Play();
    }
}

bool ComponentParticleSystem::SaveConfig(const std::string& filepath) {
    nlohmann::json j;
    Serialize(j);

    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << j.dump(4);
    file.close();
    return true;
}

bool ComponentParticleSystem::LoadConfig(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    nlohmann::json j;
    file >> j;
    file.close();

    Deserialize(j);
    return true;
}