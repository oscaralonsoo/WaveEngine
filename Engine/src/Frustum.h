#pragma once

#include <glm/glm.hpp>
#include <array>

enum PlaneIndex
{
    PLANE_NEAR = 0,
    PLANE_FAR = 1,
    PLANE_LEFT = 2,
    PLANE_RIGHT = 3,
    PLANE_TOP = 4,
    PLANE_BOTTOM = 5
};

// Frustum culling test results
enum FrustumTestResult
{
    FRUSTUM_OUT,
    FRUSTUM_INTERSECT,
    FRUSTUM_IN
};

// Plane representation using the equation: ax + by + cz + d = 0
struct Plane
{
    glm::vec3 normal;
    float distance;

    Plane();
    Plane(const glm::vec3& n, float d);

    void Normalize();
    float DistanceToPoint(const glm::vec3& point) const;
    bool IsOnPositiveSide(const glm::vec3& point) const;
};



class Frustum
{
public:
    Frustum();
    ~Frustum();

    void ExtractFromMatrix(const glm::mat4& viewProjection);
    FrustumTestResult ContainsAABB(const glm::vec3& min, const glm::vec3& max) const;
    static void GetAABBVertices(const glm::vec3& min, const glm::vec3& max, glm::vec3 corners[8]);
    const Plane& GetPlane(int index) const;

private:
    std::array<Plane, 6> planes;
};