#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "PxPhysicsAPI.h"

class PhysicsCooker {
public:

    static physx::PxConvexMesh* CookConvex(const float* vertexBuffer, uint32_t vertexCount, uint32_t stride);

    static physx::PxTriangleMesh* CookTriangleMesh(const float* vBuffer, uint32_t vCount, uint32_t vStride,
        const uint32_t* iBuffer, uint32_t iCount);
};