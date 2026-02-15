#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;

    // Color properties
    glm::vec4 color; // Current color
    glm::vec4 colorStart; // Initial color
    glm::vec4 colorEnd; // Final color, the target when interpolation is active

    // Lifecycle properties
    float lifetime; // Time remaining in seconds
    float maxLifetime; // Total life duration since creation

    // Size properties
    float size; // Current size
    float sizeStart; // Initial size
    float sizeEnd; // Final size

    // Spin properties
    float rotation; // Current rotation
    float angularVelocity; // Rotation speed

    // Animation properties
    float animationTime;

    // Rendering sorting
    float distanceToCamera;
    bool active;
};

// Forward declaration
struct EmitterInstance;

enum class ParticleModuleType {
    SPAWNER, // Initialization creation of the particle
    MOVEMENT // Handle logic behavior
};

// Shapes
enum class EmitterShape {
    BOX, // Rectangular volume
    SPHERE, // Explosions sphere
    CONE // Cone for directional spray
};

class ParticleModule {
public:
    ParticleModuleType type;
    ParticleModule(ParticleModuleType t) : type(t) {}
    virtual ~ParticleModule() = default;
    // Create when a particle is created
    virtual void Spawn(EmitterInstance* emitter, Particle* particle) = 0;
    // Updates every particle state on every frame
    virtual void Update(EmitterInstance* emitter, float dt) = 0;
    // Reset module to default
    virtual void ResetDefaults() = 0;
};

class ModuleEmitterSpawn : public ParticleModule {
public:
    glm::vec3 spawnPosition = glm::vec3(0.0f); // Default Position, centre of the object Parent

    // Lifecycle
    float lifetimeMin = 1.0f, lifetimeMax = 2.0f;
    // Physics
    float speedMin = 2.0f, speedMax = 5.0f;

    // Shapes
    EmitterShape shape = EmitterShape::BOX;
    glm::vec3 emissionArea = glm::vec3(1.0f);
    float emissionRadius = 1.0f;
    float coneAngle = 45.0f;

    // Interpolation
    float sizeStart = 0.5f, sizeEnd = 0.0f;
    glm::vec4 colorStart = glm::vec4(1.0f);
    glm::vec4 colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f); // Color fade to transparent by default

    // Rotation
    float rotationSpeedMin = 0.0f;
    float rotationSpeedMax = 0.0f;

    ModuleEmitterSpawn() : ParticleModule(ParticleModuleType::SPAWNER) {}
    void Spawn(EmitterInstance* emitter, Particle* particle) override;
    void Update(EmitterInstance* emitter, float dt) override {}
    void ResetDefaults() override;
};

class ModuleEmitterMovement : public ParticleModule {
public:
    glm::vec3 gravity = glm::vec3(0.0f, -1.0f, 0.0f);

    ModuleEmitterMovement() : ParticleModule(ParticleModuleType::MOVEMENT) {}
    void Spawn(EmitterInstance* emitter, Particle* particle) override {}
    void Update(EmitterInstance* emitter, float dt) override;
    void ResetDefaults() override;
};

struct EmitterInstance {
    // Limits and Rate
    int maxParticles = 500;
    float emissionRate = 20.0f;
    bool active = true;

    // Rendering Options
    bool additiveBlending = false; // Used for Fire or glowing items

    // Texture Resources
    unsigned int textureID = 0; // OpenGL Texture ID
    std::string texturePath;

    // Animation Configuration
    int textureRows = 1;
    int textureCols = 1;
    float animationSpeed = 1.0f; // Cycles per lifetime
    bool animLoop = false;

    // Internal State
    std::vector<Particle> particles; // The particle group
    std::vector<ParticleModule*> modules; // List of behaviors
    float timeSinceLastEmit = 0.0f; // Accumulator for emission timing

    EmitterInstance();
    ~EmitterInstance();

    void Init();
    void Update(float dt);
    void Draw(glm::vec3 cameraPosition);
    void Reset();
    void ResetValues();
    void KillDeadParticles();
    void Burst(int count); // Explosion
};