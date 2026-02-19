#define GLM_ENABLE_EXPERIMENTAL
#include "ParticleSystem.h"
#include <glad/glad.h>
#include <algorithm>
#include <cstdlib>
#include <glm/gtx/vector_angle.hpp>

// Helper for random numbers
float RandomFloat(float min, float max) {
    if (max - min < 0.0001f) return min; // Avoid division by zero
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void ModuleEmitterSpawn::ResetDefaults() {
    // Reset to default paremeters
    lifetimeMin = 1.0f; lifetimeMax = 2.0f;
    speedMin = 2.0f; speedMax = 5.0f;
    shape = EmitterShape::CONE;

    // Shapes
    emissionArea = glm::vec3(1.0f); // Box
    emissionRadius = 1.0f;          // Sphere
    coneRadius = 0.5f;              // Cone Base
    coneAngle = 25.0f;              // Cone Aperture
    circleRadius = 1.0f;            // Circle
    emitFromShell = false;

    // Interpolation
    sizeStart = 0.5f; sizeEnd = 0.0f;
    colorStart = glm::vec4(1.0f);
    colorEnd = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    colorGradient.clear();

    rotationSpeedMin = 0.0f; rotationSpeedMax = 0.0f;
}

// Creation of the particle
void ModuleEmitterSpawn::Spawn(EmitterInstance* emitter, Particle* particle) {
    particle->active = true;
    float speed = RandomFloat(speedMin, speedMax);
    glm::vec3 offset(0.0f);
    glm::vec3 dir(0, 1, 0); // Default direction is up

    if (shape == EmitterShape::BOX) {
        // Random point inside a box
        offset = glm::vec3(
            RandomFloat(-emissionArea.x, emissionArea.x),
            RandomFloat(-emissionArea.y, emissionArea.y),
            RandomFloat(-emissionArea.z, emissionArea.z)
        );
        // Direction up with variation
        dir = glm::vec3(RandomFloat(-0.2f, 0.2f), 1.0f, RandomFloat(-0.2f, 0.2f));
    }
    else if (shape == EmitterShape::SPHERE) {
        // Sphere: Random point in unit vector
        glm::vec3 randomDir = glm::vec3(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f));
        if (glm::length(randomDir) > 0.01f) randomDir = glm::normalize(randomDir);
        else randomDir = glm::vec3(0, 1, 0);

        float r = emissionRadius;
        // If not Shell, randomize radius to fill the volume
        if (!emitFromShell) r *= std::cbrt(RandomFloat(0.0f, 1.0f));

        offset = randomDir * r;
        dir = randomDir; // Explosion outwards
    }
    else if (shape == EmitterShape::CONE) {
        // Cone: Advanced trigonometric logic
        float angleRad = glm::radians(coneAngle);
        float r = coneRadius;
        if (!emitFromShell) r *= sqrt(RandomFloat(0.0f, 1.0f));

        // Random polar angle (around Y circle)
        float theta = RandomFloat(0.0f, glm::two_pi<float>());

        // Position at the cone base
        float x = r * cos(theta);
        float z = r * sin(theta);
        offset = glm::vec3(x, 0.0f, z);

        // Direction: Interpolate between UP and outward direction
        glm::vec3 baseDir = glm::normalize(glm::vec3(x, 0, z)); // Outwards in XZ plane
        if (glm::length(baseDir) < 0.01f) baseDir = glm::vec3(1, 0, 0);

        // Rotate UP vector towards the cone edge by a random amount
        float tiltAngle = RandomFloat(0.0f, angleRad);

        // Rotation axis: perpendicular to base direction and UP
        glm::vec3 rotationAxis = glm::cross(glm::vec3(0, 1, 0), baseDir);
        if (glm::length(rotationAxis) < 0.01f) rotationAxis = glm::vec3(1, 0, 0);

        dir = glm::rotate(glm::vec3(0, 1, 0), tiltAngle, glm::normalize(rotationAxis));
    }
    else if (shape == EmitterShape::CIRCLE) {
        // Circle: Plane on the ground (XZ)
        float theta = RandomFloat(0.0f, glm::two_pi<float>());
        float r = circleRadius;
        if (!emitFromShell) r *= sqrt(RandomFloat(0.0f, 1.0f));

        offset = glm::vec3(r * cos(theta), 0.0f, r * sin(theta));
        dir = glm::vec3(0, 1, 0); // Goes straight up like a column or portal
    }

    if (emitter->simulationSpace == SimulationSpace::WORLD) {
        // In World space, add the current Emitter position
        particle->position = emitter->ownerPosition + offset;
    }
    else {
        // In Local space, position is relative to system center (0,0,0)
        particle->position = offset;
    }

    particle->velocity = glm::normalize(dir) * speed;

    // Initialize Properties
    particle->lifetime = RandomFloat(lifetimeMin, lifetimeMax);
    particle->maxLifetime = particle->lifetime;

    // Start and End values
    particle->sizeStart = sizeStart;
    particle->sizeEnd = sizeEnd;
    particle->size = sizeStart;

    // Color (Gradient or Simple Interpolation)
    if (!colorGradient.empty()) {
        particle->colorStart = colorGradient.front().color;
        particle->colorEnd = colorGradient.back().color;
        particle->color = particle->colorStart;
    }
    else {
        particle->colorStart = colorStart;
        particle->colorEnd = colorEnd;
        particle->color = colorStart;
    }

    // Spin
    particle->rotation = RandomFloat(0.0f, 360.0f);
    particle->angularVelocity = RandomFloat(rotationSpeedMin, rotationSpeedMax);

    // Animation
    particle->animationTime = 0.0f;
}

void ModuleEmitterMovement::ResetDefaults() {
    gravity = glm::vec3(0.0f, -1.0f, 0.0f);
}

void ModuleEmitterMovement::Update(EmitterInstance* emitter, float dt) {
    for (auto& p : emitter->particles) {
        if (!p.active) continue;
        p.velocity += gravity * dt;
        p.position += p.velocity * dt;
    }
}

void ModuleEmitterNoise::ResetDefaults() {
    strength = 1.0f;
    frequency = 0.5f;
    active = false;
}

void ModuleEmitterNoise::Update(EmitterInstance* emitter, float dt) {
    if (!active) return;

    for (auto& p : emitter->particles) {
        if (!p.active) continue;

        // Turbulence simulation using trigonometric functions (faster than Perlin)
        float timeScale = p.maxLifetime - p.lifetime; // Lived time
        float noiseArg = (p.position.x * 0.5f + p.position.z * 0.5f + timeScale) * frequency;

        glm::vec3 noiseVel;
        noiseVel.x = sin(noiseArg) * strength;
        noiseVel.y = cos(noiseArg * 0.7f) * strength * 0.5f; // Less impact on Y
        noiseVel.z = sin(noiseArg * 1.3f) * strength;

        p.position += noiseVel * dt;
    }
}

EmitterInstance::EmitterInstance() { particles.reserve(maxParticles); }

EmitterInstance::~EmitterInstance() {
    for (auto m : modules) delete m;
    modules.clear();
}

void EmitterInstance::Init() {
    if (modules.empty()) {
        modules.push_back(new ModuleEmitterSpawn());
        modules.push_back(new ModuleEmitterMovement());
        modules.push_back(new ModuleEmitterNoise()); // Add noise module
    }
}

void EmitterInstance::Reset() {
    particles.clear();
    timeSinceLastEmit = 0.0f;
    systemTime = 0.0f;

    // Reset burst counters
    for (auto& b : bursts) b.currentCycles = 0;
}

void EmitterInstance::ResetValues() {
    // Reset Global Settings
    maxParticles = 500;
    emissionRate = 20.0f;
    emissionRateDistance = 0.0f;
    active = true;
    simulationSpace = SimulationSpace::LOCAL;
    prewarm = false;

    additiveBlending = false;
    textureRows = 1;
    textureCols = 1;
    animationSpeed = 1.0f;
    animLoop = false;

    bursts.clear();

    // Reset Module Settings
    for (auto mod : modules) {
        mod->ResetDefaults();
    }
}

// Gradient Interpolation
glm::vec4 EmitterInstance::EvaluateGradient(float t, std::vector<ColorKey>& gradient) {
    if (gradient.empty()) return glm::vec4(1.0f);
    if (gradient.size() == 1) return gradient[0].color;

    // t must be between 0 and 1
    t = glm::clamp(t, 0.0f, 1.0f);

    // Find which two keys we are between
    // case 1 before the first key
    if (t <= gradient.front().time) return gradient.front().color;

    // Case 2 after the last key
    if (t >= gradient.back().time) return gradient.back().color;

    // Case 3 inmediate interpolation
    for (size_t i = 0; i < gradient.size() - 1; ++i) {
        if (t >= gradient[i].time && t <= gradient[i + 1].time) {
            float range = gradient[i + 1].time - gradient[i].time;
            if (range <= 0.0001f) return gradient[i].color; // Avoid divisin by 0
            float localT = (t - gradient[i].time) / range;
            return glm::mix(gradient[i].color, gradient[i + 1].color, localT);
        }
    }
    return gradient.back().color;
}

void EmitterInstance::Update(float dt) {
    if (!active) return;

    systemTime += dt;
    if (emissionRateDistance > 0.0f && simulationSpace == SimulationSpace::WORLD) {
        float dist = glm::distance(ownerPosition, lastPosition);
        // Avoid calculation if we haven't moved or if it's the first frame
        if (dist > 0.001f && dist < 100.0f) { // Upper limit to avoid teleport artifacts
            int count = (int)(dist * emissionRateDistance);

            // Limit the max particles by movement physics to avoid crash or saturing
            if (count > 200) count = 200;

            // Interpolate positions to fill the gap
            glm::vec3 startPos = lastPosition;
            glm::vec3 endPos = ownerPosition;

            // Temporary, modify ownerPosition to trick the Spawn function
            for (int i = 0; i < count; i++) {
                if (particles.size() >= (size_t)maxParticles) break;

                float t = (float)(i + 1) / (float)(count + 1);
                ownerPosition = glm::mix(startPos, endPos, t);

                Particle p;
                for (auto mod : modules) mod->Spawn(this, &p);
                particles.push_back(p);
            }
            ownerPosition = endPos; // Restore real position
        }
    }
    lastPosition = ownerPosition;

    float prevSystemTime = systemTime - dt; // Time before this frame

    for (auto& burst : bursts) {
        if (burst.cycles > 0 && burst.currentCycles >= burst.cycles) continue;

        float triggerTime = burst.time + (burst.repeatInterval * burst.currentCycles);

        // Small window to detect the correct frame
        if (prevSystemTime <= triggerTime && systemTime > triggerTime) {
            Burst(burst.count);
            burst.currentCycles++;
        }
        // if the trigger is 0.0 and is the first real frame
        else if (triggerTime == 0.0f && prevSystemTime <= 0.0f && systemTime > 0.0f) {
            Burst(burst.count);
            burst.currentCycles++;
        }
    }

    if (emissionRate > 0.0f) {
        timeSinceLastEmit += dt;
        float emitInterval = 1.0f / emissionRate;
        while (timeSinceLastEmit >= emitInterval) {
            if (particles.size() < (size_t)maxParticles) {
                Particle p;
                for (auto mod : modules) mod->Spawn(this, &p);
                particles.push_back(p);
            }
            timeSinceLastEmit -= emitInterval;
        }
    }

    // Get pointer to Spawner to access the Gradient
    ModuleEmitterSpawn* spawner = nullptr;
    for (auto m : modules) if (m->type == ParticleModuleType::SPAWNER) spawner = (ModuleEmitterSpawn*)m;

    for (auto& p : particles) {
        if (!p.active) continue;

        p.lifetime -= dt;
        if (p.lifetime <= 0.0f) {
            p.active = false;
            continue;
        }

        float lifeRatio = 1.0f - (p.lifetime / p.maxLifetime);

        // Update Color (Gradient or simple interpolation)
        if (spawner && !spawner->colorGradient.empty()) {
            p.color = EvaluateGradient(lifeRatio, spawner->colorGradient);
        }
        else {
            p.color = glm::mix(p.colorStart, p.colorEnd, lifeRatio);
        }

        p.size = glm::mix(p.sizeStart, p.sizeEnd, lifeRatio);
        p.rotation += p.angularVelocity * dt;

        // Animation time
        p.animationTime = lifeRatio * animationSpeed;
    }

    // Update modules (Movement, Noise)
    for (auto mod : modules) mod->Update(this, dt);

    // Cleanup
    KillDeadParticles();
}

void EmitterInstance::KillDeadParticles() {
    // Remove all dead particles
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return !p.active; }), particles.end());
}

// Explosion effect
void EmitterInstance::Burst(int count) {
    if (!active) return;

    for (int i = 0; i < count; ++i) {
        if (particles.size() < (size_t)maxParticles) {
            Particle p;
            // Force spawn
            for (auto mod : modules) mod->Spawn(this, &p);
            particles.push_back(p);
        }
    }
}

void EmitterInstance::Draw(glm::vec3 cameraPos) {
    if (particles.empty()) return;

    // Draw far to near (Z-Sorting for transparency)
    for (auto& p : particles) p.distanceToCamera = glm::distance(p.position, cameraPos);
    std::sort(particles.begin(), particles.end(), [](const Particle& a, const Particle& b) {
        return a.distanceToCamera > b.distanceToCamera;
        });

    // Render state
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE); // Disable for transparent objects
    glDisable(GL_CULL_FACE);

    // Blending Mode
    if (additiveBlending) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Glow
    }
    else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard Alpha
    }

    // Texture Setup
    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // Multiply Texture * Color

    if (textureID != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    else {
        glDisable(GL_TEXTURE_2D);
    }

    glBegin(GL_QUADS);

    // Animation frame sizes
    int totalFrames = textureRows * textureCols;
    float frameWidth = 1.0f / (float)textureCols;
    float frameHeight = 1.0f / (float)textureRows;

    for (const auto& p : particles) {
        // BILLBOARDING
        // Coordinate system aligned with the camera
        glm::vec3 direction = glm::normalize(cameraPos - p.position);
        glm::vec3 worldUp = (glm::abs(direction.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::vec3 baseRight = glm::normalize(glm::cross(worldUp, direction));
        glm::vec3 baseUp = glm::normalize(glm::cross(direction, baseRight));

        // Apply Particle Rotation
        float rad = glm::radians(p.rotation);
        float c = cos(rad);
        float s = sin(rad);

        // Rotate Vectors
        glm::vec3 right = baseRight * c + baseUp * s;
        glm::vec3 up = baseRight * -s + baseUp * c;

        float halfSize = p.size * 0.5f;

        // Vertex Positions
        glm::vec3 v1 = p.position - (right * halfSize) + (up * halfSize);
        glm::vec3 v2 = p.position + (right * halfSize) + (up * halfSize);
        glm::vec3 v3 = p.position + (right * halfSize) - (up * halfSize);
        glm::vec3 v4 = p.position - (right * halfSize) - (up * halfSize);

        // UV animation
        int currentFrame = 0;
        if (totalFrames > 1) {
            if (animLoop) {
                currentFrame = (int)(p.animationTime * totalFrames) % totalFrames;
            }
            else {
                currentFrame = (int)(p.animationTime * totalFrames);
                if (currentFrame >= totalFrames) currentFrame = totalFrames - 1;
            }
        }

        int col = currentFrame % textureCols;
        int row = currentFrame / textureCols;

        // Map UV coordinates based on grid position
        float uLeft = col * frameWidth;
        float uRight = uLeft + frameWidth;
        float vTop = 1.0f - (row * frameHeight);
        float vBottom = vTop - frameHeight;

        glColor4f(p.color.r, p.color.g, p.color.b, p.color.a);

        glTexCoord2f(uLeft, vTop);    glVertex3f(v1.x, v1.y, v1.z);
        glTexCoord2f(uRight, vTop);   glVertex3f(v2.x, v2.y, v2.z);
        glTexCoord2f(uRight, vBottom); glVertex3f(v3.x, v3.y, v3.z);
        glTexCoord2f(uLeft, vBottom);  glVertex3f(v4.x, v4.y, v4.z);
    }

    glEnd();

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}