#pragma once

#include "Component.h"
#include <vector>
#include <glm/glm.hpp>
#include <string>

// Estructura de una partícula individual
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float lifetime;
    float maxLifetime;
    float size;
    float rotation;
    float rotationSpeed;
    bool active;

    Particle()
        : position(0.0f), velocity(0.0f), color(1.0f),
        lifetime(0.0f), maxLifetime(1.0f), size(1.0f),
        rotation(0.0f), rotationSpeed(0.0f), active(false) {
    }
};

// Configuración del emisor de partículas
struct ParticleEmitterConfig {
    // Emisión
    float emissionRate = 10.0f;  // partículas por segundo
    int maxParticles = 100;

    // Tiempo de vida
    float lifetime = 2.0f;
    float lifetimeVariance = 0.5f;

    // Velocidad
    float speed = 2.0f;
    float speedVariance = 1.0f;

    // Dirección (en grados)
    float angle = 90.0f;  // Ángulo vertical (0=derecha, 90=arriba, 180=izquierda, 270=abajo)
    float angleSpread = 30.0f;  // Apertura del cono

    // Tamaño
    float size = 0.5f;
    float sizeVariance = 0.2f;
    float sizeOverLifetime = 1.0f;  // Multiplicador al final de la vida

    // Color
    glm::vec4 startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

    // Física
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);

    // Rotación
    float rotationSpeed = 0.0f;
    float rotationSpeedVariance = 0.0f;

    // Loop
    bool loop = true;
    float duration = 5.0f;  // Duración si no es loop

    // Textura
    std::string texturePath = "";
    unsigned long long textureUID = 0;
    bool useAdditiveBlending = false;
};

class ComponentParticleSystem : public Component {
public:
    ComponentParticleSystem(GameObject* owner);
    ~ComponentParticleSystem();

    void Update() override;
    void OnEditor() override;

    // Serialization
    void Serialize(nlohmann::json& componentObj) const override;
    void Deserialize(const nlohmann::json& componentObj) override;

    // Control
    void Play();
    void Stop();
    void Pause();
    void Reset();

    bool IsPlaying() const { return isPlaying; }
    bool IsPaused() const { return isPaused; }

    // Configuración
    ParticleEmitterConfig& GetConfig() { return config; }
    const ParticleEmitterConfig& GetConfig() const { return config; }
    void SetConfig(const ParticleEmitterConfig& newConfig) { config = newConfig; }

    // Texture
    bool LoadTexture(const std::string& path);
    bool HasTexture() const { return config.textureUID != 0; }

    // Particles
    const std::vector<Particle>& GetParticles() const { return particles; }
    int GetActiveParticleCount() const;

    // Presets
    void LoadSmokePreset();
    void LoadFirePreset();
    void LoadExplosionPreset();
    void LoadSparklesPreset();

    // Save/Load config
    bool SaveConfig(const std::string& filepath);
    bool LoadConfig(const std::string& filepath);

private:
    void EmitParticle();
    void UpdateParticles(float deltaTime);
    float RandomRange(float min, float max);
    glm::vec3 GetRandomDirection();

    ParticleEmitterConfig config;
    std::vector<Particle> particles;

    bool isPlaying;
    bool isPaused;
    float accumulatedTime;
    float emitterTime;
};