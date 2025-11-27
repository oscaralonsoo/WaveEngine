// Globals.h
#pragma once
#define _HAS_STD_BYTE 0

#include <glm/glm.hpp>
#include <limits>
#include <cmath>

static unsigned int Uint;
static unsigned int uint;

// Ray-AABB intersection using slab method
inline bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
    const glm::vec3& aabbMin, const glm::vec3& aabbMax,
    float& distance)
{
    // Compute inverse direction to avoid division in loop
    glm::vec3 invDir;
    invDir.x = (std::abs(rayDir.x) > 0.0001f) ? 1.0f / rayDir.x : std::numeric_limits<float>::max();
    invDir.y = (std::abs(rayDir.y) > 0.0001f) ? 1.0f / rayDir.y : std::numeric_limits<float>::max();
    invDir.z = (std::abs(rayDir.z) > 0.0001f) ? 1.0f / rayDir.z : std::numeric_limits<float>::max();

    glm::vec3 t0 = (aabbMin - rayOrigin) * invDir;
    glm::vec3 t1 = (aabbMax - rayOrigin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    if (tNear > tFar || tFar < 0.0f)
    {
        return false;
    }

    distance = (tNear > 0.0f) ? tNear : tFar;
    return true;
}