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

ComponentParticleSystem::ComponentParticleSystem(GameObject* owner)
    : Component(owner, ComponentType::PARTICLE_SYSTEM),
    isPlaying(false), isPaused(false),
    accumulatedTime(0.0f), emitterTime(0.0f)
{
    name = "Particle System";
    particles.reserve(config.maxParticles);
}

ComponentParticleSystem::~ComponentParticleSystem() {
    // Release texture if any
    if (config.textureUID != 0) {
        Application::GetInstance().resources->ReleaseResource(config.textureUID);
    }
}

void ComponentParticleSystem::Update() {
    if (!isPlaying || isPaused) return;

    float deltaTime = Application::GetInstance().time->GetDeltaTime();

    // Update emitter time
    emitterTime += deltaTime;

    // Check duration for non-looping emitters
    if (!config.loop && emitterTime >= config.duration) {
        Stop();
        return;
    }

    // Emit new particles
    if (config.loop || emitterTime < config.duration) {
        accumulatedTime += deltaTime;
        float emitInterval = 1.0f / config.emissionRate;

        while (accumulatedTime >= emitInterval) {
            EmitParticle();
            accumulatedTime -= emitInterval;
        }
    }

    // Update existing particles
    UpdateParticles(deltaTime);
}

void ComponentParticleSystem::EmitParticle() {
    // Find inactive particle or create new one
    Particle* particle = nullptr;

    for (auto& p : particles) {
        if (!p.active) {
            particle = &p;
            break;
        }
    }

    if (!particle && particles.size() < static_cast<size_t>(config.maxParticles)) {
        particles.emplace_back();
        particle = &particles.back();
    }

    if (!particle) return;  // Max particles reached

    // Get emitter world position
    Transform* transform = static_cast<Transform*>(owner->GetComponent(ComponentType::TRANSFORM));
    glm::vec3 emitterPos = transform ? glm::vec3(transform->GetGlobalMatrix()[3]) : glm::vec3(0.0f);

    // Initialize particle
    particle->active = true;
    particle->position = emitterPos;
    particle->velocity = GetRandomDirection() * (config.speed + RandomRange(-config.speedVariance, config.speedVariance));
    particle->lifetime = 0.0f;
    particle->maxLifetime = config.lifetime + RandomRange(-config.lifetimeVariance, config.lifetimeVariance);
    if (particle->maxLifetime < 0.1f) particle->maxLifetime = 0.1f;
    particle->size = config.size + RandomRange(-config.sizeVariance, config.sizeVariance);
    if (particle->size < 0.01f) particle->size = 0.01f;
    particle->rotation = RandomRange(0.0f, 360.0f);
    particle->rotationSpeed = config.rotationSpeed + RandomRange(-config.rotationSpeedVariance, config.rotationSpeedVariance);
    particle->color = config.startColor;
}

void ComponentParticleSystem::UpdateParticles(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.active) continue;

        // Update lifetime
        particle.lifetime += deltaTime;

        if (particle.lifetime >= particle.maxLifetime) {
            particle.active = false;
            continue;
        }

        // Update position
        particle.position += particle.velocity * deltaTime;

        // Apply gravity
        particle.velocity += config.gravity * deltaTime;

        // Update rotation
        particle.rotation += particle.rotationSpeed * deltaTime;

        // Interpolate color
        float t = particle.lifetime / particle.maxLifetime;
        particle.color = glm::mix(config.startColor, config.endColor, t);

        // Scale size over lifetime
        float sizeScale = glm::mix(1.0f, config.sizeOverLifetime, t);
        float baseSize = config.size;
        particle.size = baseSize * sizeScale;
    }
}

glm::vec3 ComponentParticleSystem::GetRandomDirection() {
    // Convert angle to radians
    float angleRad = glm::radians(config.angle);
    float spreadRad = glm::radians(config.angleSpread);

    // Random deviation
    float randomAngle = angleRad + RandomRange(-spreadRad, spreadRad);
    float randomPhi = RandomRange(0.0f, glm::two_pi<float>());

    // Calculate direction vector
    glm::vec3 direction;
    direction.x = cos(randomAngle) * cos(randomPhi);
    direction.y = sin(randomAngle);
    direction.z = cos(randomAngle) * sin(randomPhi);

    return glm::normalize(direction);
}

float ComponentParticleSystem::RandomRange(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

void ComponentParticleSystem::Play() {
    isPlaying = true;
    isPaused = false;
    emitterTime = 0.0f;
    accumulatedTime = 0.0f;
}

void ComponentParticleSystem::Stop() {
    isPlaying = false;
    isPaused = false;

    // Deactivate all particles
    for (auto& p : particles) {
        p.active = false;
    }
}

void ComponentParticleSystem::Pause() {
    isPaused = !isPaused;
}

void ComponentParticleSystem::Reset() {
    Stop();
    particles.clear();
    emitterTime = 0.0f;
    accumulatedTime = 0.0f;
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

    // Release old texture
    if (config.textureUID != 0) {
        resources->ReleaseResource(config.textureUID);
        config.textureUID = 0;
    }

    // Import new texture
    UID uid = resources->Find(path.c_str());
    if (uid == 0) {
        uid = resources->ImportFile(path.c_str());
    }

    if (uid == 0) return false;

    config.textureUID = uid;
    config.texturePath = path;
    return true;
}

// Presets
void ComponentParticleSystem::LoadSmokePreset() {
    config.emissionRate = 10.0f;
    config.maxParticles = 50;
    config.lifetime = 3.0f;
    config.lifetimeVariance = 0.5f;
    config.speed = 1.0f;
    config.speedVariance = 0.3f;
    config.angle = 90.0f;
    config.angleSpread = 20.0f;
    config.size = 1.0f;
    config.sizeVariance = 0.3f;
    config.sizeOverLifetime = 2.0f;
    config.startColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);
    config.endColor = glm::vec4(0.2f, 0.2f, 0.2f, 0.0f);
    config.gravity = glm::vec3(0.0f, 0.5f, 0.0f);
    config.rotationSpeed = 20.0f;
    config.rotationSpeedVariance = 10.0f;
    config.loop = true;
    config.useAdditiveBlending = false;
}

void ComponentParticleSystem::LoadFirePreset() {
    config.emissionRate = 20.0f;
    config.maxParticles = 100;
    config.lifetime = 1.5f;
    config.lifetimeVariance = 0.3f;
    config.speed = 2.0f;
    config.speedVariance = 0.5f;
    config.angle = 90.0f;
    config.angleSpread = 15.0f;
    config.size = 0.5f;
    config.sizeVariance = 0.2f;
    config.sizeOverLifetime = 0.5f;
    config.startColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
    config.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    config.gravity = glm::vec3(0.0f, 1.0f, 0.0f);
    config.rotationSpeed = 50.0f;
    config.rotationSpeedVariance = 30.0f;
    config.loop = true;
    config.useAdditiveBlending = true;
}

void ComponentParticleSystem::LoadExplosionPreset() {
    config.emissionRate = 100.0f;
    config.maxParticles = 200;
    config.lifetime = 2.0f;
    config.lifetimeVariance = 0.5f;
    config.speed = 5.0f;
    config.speedVariance = 2.0f;
    config.angle = 90.0f;
    config.angleSpread = 180.0f;  // Todas direcciones
    config.size = 0.3f;
    config.sizeVariance = 0.1f;
    config.sizeOverLifetime = 0.2f;
    config.startColor = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f);
    config.endColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    config.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    config.rotationSpeed = 100.0f;
    config.rotationSpeedVariance = 50.0f;
    config.loop = false;
    config.duration = 0.5f;
    config.useAdditiveBlending = true;
}

void ComponentParticleSystem::LoadSparklesPreset() {
    config.emissionRate = 30.0f;
    config.maxParticles = 150;
    config.lifetime = 2.0f;
    config.lifetimeVariance = 0.5f;
    config.speed = 1.0f;
    config.speedVariance = 0.5f;
    config.angle = 90.0f;
    config.angleSpread = 30.0f;
    config.size = 0.2f;
    config.sizeVariance = 0.1f;
    config.sizeOverLifetime = 0.5f;
    config.startColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    config.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    config.gravity = glm::vec3(0.0f, -2.0f, 0.0f);
    config.rotationSpeed = 200.0f;
    config.rotationSpeedVariance = 100.0f;
    config.loop = true;
    config.useAdditiveBlending = true;
}

void ComponentParticleSystem::Serialize(nlohmann::json& componentObj) const {
    componentObj["emissionRate"] = config.emissionRate;
    componentObj["maxParticles"] = config.maxParticles;
    componentObj["lifetime"] = config.lifetime;
    componentObj["lifetimeVariance"] = config.lifetimeVariance;
    componentObj["speed"] = config.speed;
    componentObj["speedVariance"] = config.speedVariance;
    componentObj["angle"] = config.angle;
    componentObj["angleSpread"] = config.angleSpread;
    componentObj["size"] = config.size;
    componentObj["sizeVariance"] = config.sizeVariance;
    componentObj["sizeOverLifetime"] = config.sizeOverLifetime;

    componentObj["startColor"] = {
        config.startColor.r, config.startColor.g,
        config.startColor.b, config.startColor.a
    };
    componentObj["endColor"] = {
        config.endColor.r, config.endColor.g,
        config.endColor.b, config.endColor.a
    };

    componentObj["gravity"] = {
        config.gravity.x, config.gravity.y, config.gravity.z
    };

    componentObj["rotationSpeed"] = config.rotationSpeed;
    componentObj["rotationSpeedVariance"] = config.rotationSpeedVariance;
    componentObj["loop"] = config.loop;
    componentObj["duration"] = config.duration;
    componentObj["textureUID"] = config.textureUID;
    componentObj["texturePath"] = config.texturePath;
    componentObj["useAdditiveBlending"] = config.useAdditiveBlending;
    componentObj["isPlaying"] = isPlaying;
}

void ComponentParticleSystem::Deserialize(const nlohmann::json& componentObj) {
    if (componentObj.contains("emissionRate")) config.emissionRate = componentObj["emissionRate"];
    if (componentObj.contains("maxParticles")) config.maxParticles = componentObj["maxParticles"];
    if (componentObj.contains("lifetime")) config.lifetime = componentObj["lifetime"];
    if (componentObj.contains("lifetimeVariance")) config.lifetimeVariance = componentObj["lifetimeVariance"];
    if (componentObj.contains("speed")) config.speed = componentObj["speed"];
    if (componentObj.contains("speedVariance")) config.speedVariance = componentObj["speedVariance"];
    if (componentObj.contains("angle")) config.angle = componentObj["angle"];
    if (componentObj.contains("angleSpread")) config.angleSpread = componentObj["angleSpread"];
    if (componentObj.contains("size")) config.size = componentObj["size"];
    if (componentObj.contains("sizeVariance")) config.sizeVariance = componentObj["sizeVariance"];
    if (componentObj.contains("sizeOverLifetime")) config.sizeOverLifetime = componentObj["sizeOverLifetime"];

    if (componentObj.contains("startColor")) {
        auto c = componentObj["startColor"];
        config.startColor = glm::vec4(c[0], c[1], c[2], c[3]);
    }
    if (componentObj.contains("endColor")) {
        auto c = componentObj["endColor"];
        config.endColor = glm::vec4(c[0], c[1], c[2], c[3]);
    }
    if (componentObj.contains("gravity")) {
        auto g = componentObj["gravity"];
        config.gravity = glm::vec3(g[0], g[1], g[2]);
    }

    if (componentObj.contains("rotationSpeed")) config.rotationSpeed = componentObj["rotationSpeed"];
    if (componentObj.contains("rotationSpeedVariance")) config.rotationSpeedVariance = componentObj["rotationSpeedVariance"];
    if (componentObj.contains("loop")) config.loop = componentObj["loop"];
    if (componentObj.contains("duration")) config.duration = componentObj["duration"];
    if (componentObj.contains("textureUID")) config.textureUID = componentObj["textureUID"];
    if (componentObj.contains("texturePath")) config.texturePath = componentObj["texturePath"];
    if (componentObj.contains("useAdditiveBlending")) config.useAdditiveBlending = componentObj["useAdditiveBlending"];

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

void ComponentParticleSystem::OnEditor() {
    // Implementación vacía, se maneja en InspectorWindow
}