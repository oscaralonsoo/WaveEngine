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

// Enums para las diferentes formas de emisión
enum class EmitterShape {
    CONE,
    SPHERE,
    HEMISPHERE,
    CIRCLE,
    BOX,
    EDGE
};

enum class EmissionMode {
    RANDOM,
    LOOP,
    PING_PONG
};

// Módulo de emisión
struct EmissionModule {
    bool enabled = true;
    float rateOverTime = 10.0f;
    float rateOverDistance = 0.0f;

    struct Burst {
        float time = 0.0f;
        int count = 30;
        int cycles = 1;
        float repeatInterval = 1.0f;
    };
    std::vector<Burst> bursts;
};

// Módulo de forma (Shape)
struct ShapeModule {
    bool enabled = true;
    EmitterShape shape = EmitterShape::CONE;

    // Cone
    float coneAngle = 25.0f;
    float coneRadius = 1.0f;
    float coneLength = 1.0f;
    bool emitFromBase = false;

    // Sphere/Hemisphere
    float sphereRadius = 1.0f;
    float sphereRadiusThickness = 1.0f;

    // Circle
    float circleRadius = 1.0f;
    float circleArc = 360.0f;
    EmissionMode circleMode = EmissionMode::RANDOM;
    float circleSpeed = 1.0f;
    float circleSpread = 0.0f;

    // Box
    glm::vec3 boxSize = glm::vec3(1.0f);
    bool emitFromVolume = true;

    // Edge
    float edgeRadius = 1.0f;

    // General
    float randomDirectionAmount = 0.0f;
    float sphericalDirectionAmount = 0.0f;
};

// Módulo de velocidad a lo largo de la vida
struct VelocityOverLifetimeModule {
    bool enabled = false;
    glm::vec3 velocity = glm::vec3(0.0f);
    bool isLocal = false;
};

// Módulo de límite de velocidad
struct LimitVelocityOverLifetimeModule {
    bool enabled = false;
    float speed = 1.0f;
    float dampen = 0.5f;
};

// Módulo de fuerza
struct ForceOverLifetimeModule {
    bool enabled = false;
    glm::vec3 force = glm::vec3(0.0f);
    bool isLocal = false;
};

// Módulo de color
struct ColorOverLifetimeModule {
    bool enabled = true;
    glm::vec4 startColor = glm::vec4(1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);

    struct ColorKey {
        float time;
        glm::vec4 color;
    };
    std::vector<ColorKey> gradient;
};

// Módulo de color por velocidad
struct ColorBySpeedModule {
    bool enabled = false;
    glm::vec4 minColor = glm::vec4(1.0f);
    glm::vec4 maxColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    float minSpeed = 0.0f;
    float maxSpeed = 10.0f;
};

// Módulo de tamaño
struct SizeOverLifetimeModule {
    bool enabled = true;
    float startSize = 1.0f;
    float endSize = 0.5f;

    struct SizeKey {
        float time;
        float size;
    };
    std::vector<SizeKey> sizeCurve;
};

// Módulo de tamaño por velocidad
struct SizeBySpeedModule {
    bool enabled = false;
    float minSize = 0.5f;
    float maxSize = 2.0f;
    float minSpeed = 0.0f;
    float maxSpeed = 10.0f;
};

// Módulo de rotación
struct RotationOverLifetimeModule {
    bool enabled = false;
    float angularVelocity = 0.0f;
    float angularVelocityVariance = 0.0f;
};

// Módulo de rotación por velocidad
struct RotationBySpeedModule {
    bool enabled = false;
    float minAngularVelocity = 0.0f;
    float maxAngularVelocity = 360.0f;
    float minSpeed = 0.0f;
    float maxSpeed = 10.0f;
};

// Módulo de animación de textura
struct TextureSheetAnimationModule {
    bool enabled = false;
    int tilesX = 1;
    int tilesY = 1;
    int cycles = 1;
    float frameOverTime = 1.0f;
    bool randomRow = false;
};

// Módulo de ruido
struct NoiseModule {
    bool enabled = false;
    float strength = 1.0f;
    float frequency = 0.5f;
    float scrollSpeed = 0.0f;
    bool damping = true;
};

// Configuración principal
struct ParticleSystemConfig {
    // Main Module
    float duration = 5.0f;
    bool looping = true;
    bool prewarm = false;
    float startDelay = 0.0f;

    float startLifetime = 5.0f;
    float startLifetimeVariance = 0.0f;

    float startSpeed = 5.0f;
    float startSpeedVariance = 0.0f;

    float startSize = 1.0f;
    float startSizeVariance = 0.0f;

    float startRotation = 0.0f;
    float startRotationVariance = 0.0f;

    glm::vec4 startColor = glm::vec4(1.0f);

    float gravityModifier = 0.0f;

    enum class SimulationSpace {
        LOCAL,
        WORLD
    };
    SimulationSpace simulationSpace = SimulationSpace::LOCAL;

    bool playOnAwake = true;
    int maxParticles = 1000;

    // Rendering
    enum class RenderMode {
        BILLBOARD,
        STRETCHED_BILLBOARD,
        HORIZONTAL_BILLBOARD,
        VERTICAL_BILLBOARD,
        MESH
    };
    RenderMode renderMode = RenderMode::BILLBOARD;

    enum class BlendMode {
        ALPHA_BLEND,
        ADDITIVE,
        SUBTRACTIVE,
        MULTIPLY
    };
    BlendMode blendMode = BlendMode::ALPHA_BLEND;

    // Textura
    std::string texturePath = "";
    unsigned long long textureUID = 0;
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
    void Restart();
    void Clear();

    bool IsPlaying() const { return isPlaying; }
    bool IsPaused() const { return isPaused; }
    bool IsAlive() const;

    // Configuración principal
    ParticleSystemConfig& GetMainConfig() { return mainConfig; }
    const ParticleSystemConfig& GetMainConfig() const { return mainConfig; }

    // Módulos
    EmissionModule& GetEmissionModule() { return emissionModule; }
    ShapeModule& GetShapeModule() { return shapeModule; }
    VelocityOverLifetimeModule& GetVelocityOverLifetime() { return velocityOverLifetime; }
    LimitVelocityOverLifetimeModule& GetLimitVelocityOverLifetime() { return limitVelocityOverLifetime; }
    ForceOverLifetimeModule& GetForceOverLifetime() { return forceOverLifetime; }
    ColorOverLifetimeModule& GetColorOverLifetime() { return colorOverLifetime; }
    ColorBySpeedModule& GetColorBySpeed() { return colorBySpeed; }
    SizeOverLifetimeModule& GetSizeOverLifetime() { return sizeOverLifetime; }
    SizeBySpeedModule& GetSizeBySpeed() { return sizeBySpeed; }
    RotationOverLifetimeModule& GetRotationOverLifetime() { return rotationOverLifetime; }
    RotationBySpeedModule& GetRotationBySpeed() { return rotationBySpeed; }
    TextureSheetAnimationModule& GetTextureSheetAnimation() { return textureSheetAnimation; }
    NoiseModule& GetNoise() { return noiseModule; }

    // Texture
    bool LoadTexture(const std::string& path);
    bool HasTexture() const { return mainConfig.textureUID != 0; }

    // Particles
    const std::vector<Particle>& GetParticles() const { return particles; }
    int GetActiveParticleCount() const;

    // Presets
    void LoadSmokePreset();
    void LoadFirePreset();
    void LoadExplosionPreset();
    void LoadSnowPreset();

    // Save/Load config
    bool SaveConfig(const std::string& filepath);
    bool LoadConfig(const std::string& filepath);

private:
    // Emisión de partículas
    void EmitParticle();
    void UpdateParticles(float deltaTime);

    // Aplicar módulos
    void ApplyVelocityOverLifetime(Particle& particle, float normalizedLifetime);
    void ApplyLimitVelocityOverLifetime(Particle& particle);
    void ApplyForceOverLifetime(Particle& particle, float deltaTime, float normalizedLifetime);
    void ApplyColorOverLifetime(Particle& particle, float normalizedLifetime);
    void ApplyColorBySpeed(Particle& particle);
    void ApplySizeOverLifetime(Particle& particle, float normalizedLifetime);
    void ApplySizeBySpeed(Particle& particle);
    void ApplyRotationOverLifetime(Particle& particle, float deltaTime, float normalizedLifetime);
    void ApplyRotationBySpeed(Particle& particle);
    void ApplyNoise(Particle& particle, float deltaTime);

    // Formas de emisión
    glm::vec3 GetEmissionPosition();
    glm::vec3 GetEmissionDirection();

    glm::vec3 GetConeEmissionPosition();
    glm::vec3 GetConeEmissionDirection();

    glm::vec3 GetSphereEmissionPosition();
    glm::vec3 GetSphereEmissionDirection();

    glm::vec3 GetCircleEmissionPosition();
    glm::vec3 GetCircleEmissionDirection();

    glm::vec3 GetBoxEmissionPosition();
    glm::vec3 GetBoxEmissionDirection();

    glm::vec3 GetEdgeEmissionPosition();
    glm::vec3 GetEdgeEmissionDirection();

    // Utilidades
    float RandomRange(float min, float max);
    glm::vec3 RandomInUnitSphere();
    glm::vec3 RandomOnUnitSphere();
    float EvaluateCurve(const std::vector<SizeOverLifetimeModule::SizeKey>& curve, float time);
    glm::vec4 EvaluateGradient(const std::vector<ColorOverLifetimeModule::ColorKey>& gradient, float time);

    // Configuración y módulos
    ParticleSystemConfig mainConfig;

    EmissionModule emissionModule;
    ShapeModule shapeModule;
    VelocityOverLifetimeModule velocityOverLifetime;
    LimitVelocityOverLifetimeModule limitVelocityOverLifetime;
    ForceOverLifetimeModule forceOverLifetime;
    ColorOverLifetimeModule colorOverLifetime;
    ColorBySpeedModule colorBySpeed;
    SizeOverLifetimeModule sizeOverLifetime;
    SizeBySpeedModule sizeBySpeed;
    RotationOverLifetimeModule rotationOverLifetime;
    RotationBySpeedModule rotationBySpeed;
    TextureSheetAnimationModule textureSheetAnimation;
    NoiseModule noiseModule;

    // Partículas
    std::vector<Particle> particles;

    // Estado
    bool isPlaying;
    bool isPaused;
    float systemTime;
    float accumulatedTime;
    glm::vec3 lastPosition;

    // Para bursts
    struct BurstState {
        int burstIndex;
        int cyclesCompleted;
        float nextBurstTime;
    };
    std::vector<BurstState> burstStates;

    // Para circle mode
    float circleEmissionAngle;
};