#include "Frustum.h"
#include <iostream>

Plane::Plane()
{
    normal = glm::vec3(0.0f, 1.0f, 0.0f);
    distance = 0.0f;
}

Plane::Plane(const glm::vec3& n, float d)
{
    normal = n;
    distance = d;
}

void Plane::Normalize()
{
    float length = glm::length(normal);
    if (length > 0.0f)
    {
        normal = normal / length;
        distance = distance / length;
    }
}

float Plane::DistanceToPoint(const glm::vec3& point) const
{
    return glm::dot(normal, point) + distance;
}

bool Plane::IsOnPositiveSide(const glm::vec3& point) const
{
    return DistanceToPoint(point) >= 0.0f;
}

Frustum::Frustum()
{
}

Frustum::~Frustum()
{
}

const Plane& Frustum::GetPlane(int index) const
{
    return planes[index];
}

void Frustum::ExtractFromMatrix(const glm::mat4& vp)
{
    // Extract frustum planes from View-Projection matrix
    // GLM uses column-major order: vp[col][row]
    // Normals point OUTWARD from frustum

    // LEFT plane (col3 + col0)
    planes[PLANE_LEFT].normal.x = vp[0][3] + vp[0][0];
    planes[PLANE_LEFT].normal.y = vp[1][3] + vp[1][0];
    planes[PLANE_LEFT].normal.z = vp[2][3] + vp[2][0];
    planes[PLANE_LEFT].distance = vp[3][3] + vp[3][0];
    planes[PLANE_LEFT].Normalize();

    // RIGHT plane (col3 - col0)
    planes[PLANE_RIGHT].normal.x = vp[0][3] - vp[0][0];
    planes[PLANE_RIGHT].normal.y = vp[1][3] - vp[1][0];
    planes[PLANE_RIGHT].normal.z = vp[2][3] - vp[2][0];
    planes[PLANE_RIGHT].distance = vp[3][3] - vp[3][0];
    planes[PLANE_RIGHT].Normalize();

    // BOTTOM plane (col3 + col1)
    planes[PLANE_BOTTOM].normal.x = vp[0][3] + vp[0][1];
    planes[PLANE_BOTTOM].normal.y = vp[1][3] + vp[1][1];
    planes[PLANE_BOTTOM].normal.z = vp[2][3] + vp[2][1];
    planes[PLANE_BOTTOM].distance = vp[3][3] + vp[3][1];
    planes[PLANE_BOTTOM].Normalize();

    // TOP plane (col3 - col1)
    planes[PLANE_TOP].normal.x = vp[0][3] - vp[0][1];
    planes[PLANE_TOP].normal.y = vp[1][3] - vp[1][1];
    planes[PLANE_TOP].normal.z = vp[2][3] - vp[2][1];
    planes[PLANE_TOP].distance = vp[3][3] - vp[3][1];
    planes[PLANE_TOP].Normalize();

    // NEAR plane (col3 + col2)
    planes[PLANE_NEAR].normal.x = vp[0][3] + vp[0][2];
    planes[PLANE_NEAR].normal.y = vp[1][3] + vp[1][2];
    planes[PLANE_NEAR].normal.z = vp[2][3] + vp[2][2];
    planes[PLANE_NEAR].distance = vp[3][3] + vp[3][2];
    planes[PLANE_NEAR].Normalize();

    // FAR plane (col3 - col2)
    planes[PLANE_FAR].normal.x = vp[0][3] - vp[0][2];
    planes[PLANE_FAR].normal.y = vp[1][3] - vp[1][2];
    planes[PLANE_FAR].normal.z = vp[2][3] - vp[2][2];
    planes[PLANE_FAR].distance = vp[3][3] - vp[3][2];
    planes[PLANE_FAR].Normalize();
}

void Frustum::GetAABBVertices(const glm::vec3& min, const glm::vec3& max, glm::vec3 corners[8])
{
    corners[0] = glm::vec3(min.x, min.y, min.z);
    corners[1] = glm::vec3(max.x, min.y, min.z);
    corners[2] = glm::vec3(min.x, max.y, min.z);
    corners[3] = glm::vec3(max.x, max.y, min.z);
    corners[4] = glm::vec3(min.x, min.y, max.z);
    corners[5] = glm::vec3(max.x, min.y, max.z);
    corners[6] = glm::vec3(min.x, max.y, max.z);
    corners[7] = glm::vec3(max.x, max.y, max.z);
}

FrustumTestResult Frustum::ContainsAABB(const glm::vec3& min, const glm::vec3& max) const
{
    glm::vec3 vCorner[8];
    GetAABBVertices(min, max, vCorner);

    int iTotalIn = 0;

    for (int p = 0; p < 6; ++p)
    {
        int iInCount = 8;
        int iPtIn = 1;

        for (int i = 0; i < 8; ++i)
        {
            // Negative distance = point is INSIDE (in front of plane)
            // Positive distance = point is OUTSIDE (behind plane)
            if (planes[p].DistanceToPoint(vCorner[i]) < 0.0f)
            {
                iPtIn = 0;
                --iInCount;
            }
        }

        if (iInCount == 0)
            return FRUSTUM_OUT;

        iTotalIn += iPtIn;
    }

    if (iTotalIn == 6)
        return FRUSTUM_IN;

    return FRUSTUM_INTERSECT;
}