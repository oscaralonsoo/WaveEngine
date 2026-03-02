#pragma once
#include "glm/glm.hpp"
#include <array>

class AABB
{
public:
    glm::vec3 min;
    glm::vec3 max;

    AABB GetGlobalAABB(const glm::mat4& modelMatrix) const
    {
        const std::array<glm::vec3, 8> localVertices = {
            glm::vec3(min.x, min.y, min.z),
            glm::vec3(max.x, min.y, min.z),
            glm::vec3(min.x, max.y, min.z),
            glm::vec3(max.x, max.y, min.z),
            glm::vec3(min.x, min.y, max.z),
            glm::vec3(max.x, min.y, max.z),
            glm::vec3(min.x, max.y, max.z),
            glm::vec3(max.x, max.y, max.z)
        };

        AABB globalAABB;

        globalAABB.min = glm::vec3(INFINITY, INFINITY, INFINITY);
        globalAABB.max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);

        for (int i = 0; i < localVertices.size(); ++i)
        {
            glm::vec3 globalVertices = glm::vec3(modelMatrix * glm::vec4(localVertices[i], 1.0f));

            globalAABB.min.x = fmin(globalAABB.min.x, globalVertices.x);
            globalAABB.min.y = fmin(globalAABB.min.y, globalVertices.y);
            globalAABB.min.z = fmin(globalAABB.min.z, globalVertices.z);
            globalAABB.max.x = fmax(globalAABB.max.x, globalVertices.x);
            globalAABB.max.y = fmax(globalAABB.max.y, globalVertices.y);
            globalAABB.max.z = fmax(globalAABB.max.z, globalVertices.z);
        }

        return globalAABB;
    }

    void SetNegativeInfinity()
    {
        min = glm::vec3(INFINITY, INFINITY, INFINITY);
        max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);
    }
    void Enclose(const glm::vec3& p)
    {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
};