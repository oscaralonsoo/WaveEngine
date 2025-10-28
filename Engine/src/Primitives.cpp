#include "Primitives.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Mesh Primitives::CreateTriangle()
{
    Mesh mesh;

    mesh.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}
    };

    mesh.indices = { 0, 1, 2 };

    return mesh;
}

Mesh Primitives::CreateCube()
{
    Mesh mesh;

    //mesh.vertices.push_back({
    //{ -0.5f, -0.5f,  0.5f },  // posición
    //{  0.0f,  0.0f,  1.0f },  // normal
    //{  0.0f,  0.0f }          // coordenadas UV
    //    });

    // Front face
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f} });

    // Back face
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} });

    // Left face
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} });

    // Right face
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f} });

    // Top face
    mesh.vertices.push_back({ {-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f} });

    // Bottom face
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f} });

    // Indices (6 faces * 2 triangles * 3 vertices = 36 indices)
    mesh.indices = {
        0, 1, 2,  2, 3, 0,      // Front
        4, 5, 6,  6, 7, 4,      // Back
        8, 9, 10, 10, 11, 8,    // Left
        12, 13, 14, 14, 15, 12, // Right
        16, 17, 18, 18, 19, 16, // Top
        20, 21, 22, 22, 23, 20  // Bottom
    };

    return mesh;
}

Mesh Primitives::CreatePyramid()
{
    Mesh mesh;

    // Front face
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.447f, 0.894f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, {0.0f, 0.447f, 0.894f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.0f,  0.5f,  0.0f}, {0.0f, 0.447f, 0.894f}, {0.5f, 1.0f} });

    // Right face
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, {0.894f, 0.447f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.894f, 0.447f, 0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.0f,  0.5f,  0.0f}, {0.894f, 0.447f, 0.0f}, {0.5f, 1.0f} });

    // Back face
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.447f, -0.894f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.447f, -0.894f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.0f,  0.5f,  0.0f}, {0.0f, 0.447f, -0.894f}, {0.5f, 1.0f} });

    // Left face
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, {-0.894f, 0.447f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {-0.894f, 0.447f, 0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.0f,  0.5f,  0.0f}, {-0.894f, 0.447f, 0.0f}, {0.5f, 1.0f} });

    // Bottom face (2 triangles)
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });

    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });

    // Indices
    for (unsigned int i = 0; i < 18; i++)
        mesh.indices.push_back(i);

    return mesh;
}

Mesh Primitives::CreatePlane(float width, float height)
{
    Mesh mesh;

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    mesh.vertices = {
        {{-halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
    };

    mesh.indices = { 0, 1, 2, 2, 3, 0 };

    return mesh;
}

Mesh Primitives::CreateSphere(float radius, unsigned int rings, unsigned int sectors)
{
    Mesh mesh;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (unsigned int r = 0; r < rings; r++)
    {
        for (unsigned int s = 0; s < sectors; s++)
        {
            float const y = sinf(-M_PI / 2.0f + M_PI * r * R);
            float const x = cosf(2.0f * M_PI * s * S) * sinf(M_PI * r * R);
            float const z = sinf(2.0f * M_PI * s * S) * sinf(M_PI * r * R);

            Vertex vertex;
            vertex.position = glm::vec3(x * radius, y * radius, z * radius);
            vertex.normal = glm::normalize(glm::vec3(x, y, z));
            vertex.texCoords = glm::vec2(s * S, r * R);

            mesh.vertices.push_back(vertex);
        }
    }

    for (unsigned int r = 0; r < rings - 1; r++)
    {
        for (unsigned int s = 0; s < sectors - 1; s++)
        {
            mesh.indices.push_back(r * sectors + s);
            mesh.indices.push_back(r * sectors + (s + 1));
            mesh.indices.push_back((r + 1) * sectors + (s + 1));

            mesh.indices.push_back(r * sectors + s);
            mesh.indices.push_back((r + 1) * sectors + (s + 1));
            mesh.indices.push_back((r + 1) * sectors + s);
        }
    }

    return mesh;
}

Mesh Primitives::CreateCylinder(float radius, float height, unsigned int segments)
{
    Mesh mesh;

    float halfHeight = height * 0.5f;

    // Top center
    mesh.vertices.push_back({ {0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });

    // Bottom center
    mesh.vertices.push_back({ {0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });

    // Top and bottom circle vertices
    for (unsigned int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;

        // Top circle
        mesh.vertices.push_back({ {x, halfHeight, z}, {0.0f, 1.0f, 0.0f}, {(float)i / segments, 0.0f} });

        // Bottom circle
        mesh.vertices.push_back({ {x, -halfHeight, z}, {0.0f, -1.0f, 0.0f}, {(float)i / segments, 1.0f} });
    }

    // Top cap indices
    for (unsigned int i = 0; i < segments; i++)
    {
        mesh.indices.push_back(0);
        mesh.indices.push_back(2 + i * 2);
        mesh.indices.push_back(2 + (i + 1) * 2);
    }

    // Bottom cap indices
    for (unsigned int i = 0; i < segments; i++)
    {
        mesh.indices.push_back(1);
        mesh.indices.push_back(3 + (i + 1) * 2);
        mesh.indices.push_back(3 + i * 2);
    }

    // Side faces indices
    for (unsigned int i = 0; i < segments; i++)
    {
        unsigned int topCurrent = 2 + i * 2;
        unsigned int bottomCurrent = 3 + i * 2;
        unsigned int topNext = 2 + (i + 1) * 2;
        unsigned int bottomNext = 3 + (i + 1) * 2;

        mesh.indices.push_back(topCurrent);
        mesh.indices.push_back(bottomCurrent);
        mesh.indices.push_back(topNext);

        mesh.indices.push_back(topNext);
        mesh.indices.push_back(bottomCurrent);
        mesh.indices.push_back(bottomNext);
    }

    return mesh;
}