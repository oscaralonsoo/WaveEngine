#pragma once

#include <vector>
#include <string>
#include <map> 
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Gradient
struct ColorKey {
    float time; // Position from 0.0 to 1.0
    glm::vec4 color;
};

// Repeated Burst
struct Burst {
    float time = 0.0f; // Time at which it occurs
    int count = 10; // Number of particles
    int cycles = 1; // How many times does it repeat
    float repeatInterval = 1.0f; // Time between repetitions

    // Internal control variables
    int currentCycles = 0;
};

// SimulationSpace
enum class SimulationSpace {
    LOCAL, // The particles move with the object
    WORLD // The particles remain in the world, leaving a trail
};

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
    MOVEMENT, // Handle logic behavior
    NOISE
};

// Shapes
enum class EmitterShape {
    BOX, // Rectangular volume
    SPHERE, // Explosions sphere
    CONE, // Cone for directional spray
    CIRCLE // Flat circular
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

    // Shape default
    EmitterShape shape = EmitterShape::CONE;

    // Shapes Properties
    glm::vec3 emissionArea = glm::vec3(1.0f); // For box
    float emissionRadius = 1.0f; // For Sphere
    float coneRadius = 0.5f;   // Radius of the cone base
    float coneAngle = 25.0f;   // Cone opening angle
    float circleRadius = 1.0f; // Radius of the ring/circle
    bool emitFromShell = false; // If its true, emits only from the edge

    // Interpolation
    float sizeStart = 0.5f, sizeEnd = 0.0f;

    // Color Simple
    glm::vec4 colorStart = glm::vec4(1.0f);
    glm::vec4 colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f); // Color fade to transparent by default

    // Advanced interpolation with color gradients
    std::vector<ColorKey> colorGradient;

    // Rotation
    float rotationSpeedMin = 0.0f;
    float rotationSpeedMax = 0.0f;

    ModuleEmitterSpawn() : ParticleModule(ParticleModuleType::SPAWNER) {}
    void Spawn(EmitterInstance* emitter, Particle* particle) override;
    void Update(EmitterInstance* emitter, float dt) override {}
    void ResetDefaults() override;
};

// Basic gravity
class ModuleEmitterMovement : public ParticleModule {
public:
    glm::vec3 gravity = glm::vec3(0.0f, -1.0f, 0.0f);

    ModuleEmitterMovement() : ParticleModule(ParticleModuleType::MOVEMENT) {}
    void Spawn(EmitterInstance* emitter, Particle* particle) override {}
    void Update(EmitterInstance* emitter, float dt) override;
    void ResetDefaults() override;
};

// NOISE: Turbulence
class ModuleEmitterNoise : public ParticleModule {
public:
    bool active = false;
    float strength = 1.0f; // Noise strength
    float frequency = 0.5f; // Frequency, how fast it changes

    ModuleEmitterNoise() : ParticleModule(ParticleModuleType::NOISE) {}
    void Spawn(EmitterInstance* emitter, Particle* particle) override {}
    void Update(EmitterInstance* emitter, float dt) override;
    void ResetDefaults() override;
};

struct EmitterInstance {
    // Limits and Rate
    int maxParticles = 500;
    float emissionRate = 20.0f; // Time-based emission
    bool active = true;

    // Simulation Settings
    SimulationSpace simulationSpace = SimulationSpace::LOCAL;
    float emissionRateDistance = 0.0f; // Emission per distance traveled
    bool prewarm = false; // Pre-warm up

    // Bursts
    std::vector<Burst> bursts;

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

    // State variables for calculations
    float systemTime = 0.0f; // System uptime
    glm::vec3 ownerPosition = glm::vec3(0.0f);// Current position of the owner object
    glm::vec3 lastPosition = glm::vec3(0.0f); // Position in the previous frame

    EmitterInstance();
    ~EmitterInstance();

    void Init();
    void Update(float dt);
    void Draw(glm::vec3 cameraPosition);
    void Reset();
    void ResetValues();
    void KillDeadParticles();
    void Burst(int count); // Explosion

    // Helper for gradients
    glm::vec4 EvaluateGradient(float t, std::vector<ColorKey>& gradient);
};