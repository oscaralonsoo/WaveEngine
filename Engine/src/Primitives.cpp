#include "Primitives.h"
#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentBoxCollider.h"
#include "ComponentRigidBody.h"
#include "Application.h"
#include "ModuleScene.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

GameObject* Primitives::CreateCubeGameObject(const std::string& name, float mass)
{
    // 1. Creamos el objeto a través del ModuleScene del App
    GameObject* go = Application::GetInstance().scene->CreateGameObject(name);

    // 2. Añadimos la malla visual
    ComponentMesh* mesh = (ComponentMesh*)go->CreateComponent(ComponentType::MESH);
    if (mesh) mesh->SetMesh(Primitives::CreateCube());

    // 3. Añadimos el Collider (Caja para el cubo)
    ComponentBoxCollider* boxCol = (ComponentBoxCollider*)go->CreateComponent(ComponentType::COLLIDER_BOX);
    if (boxCol) boxCol->SetSize(glm::vec3(1.0f, 1.0f, 1.0f));

    // 4. Añadimos el RigidBody
    ComponentRigidBody* rb = (ComponentRigidBody*)go->CreateComponent(ComponentType::RIGIDBODY);
    if (rb) {
        rb->SetMass(mass);
        rb->Start();
    }

    return go;
}

GameObject* Primitives::CreateSphereGameObject(const std::string& name, float mass)
{
    GameObject* go = Application::GetInstance().scene->CreateGameObject(name);

    ComponentMesh* mesh = (ComponentMesh*)go->CreateComponent(ComponentType::MESH);
    if (mesh) mesh->SetMesh(Primitives::CreateSphere());

    // 3. Añadimos el Collider (Asumiendo que tienes COLLIDER_SPHERE)
    // ComponentSphereCollider* sphereCol = (ComponentSphereCollider*)go->CreateComponent(ComponentType::COLLIDER_SPHERE);
    // if (sphereCol) sphereCol->SetRadius(0.5f);

    ComponentRigidBody* rb = (ComponentRigidBody*)go->CreateComponent(ComponentType::RIGIDBODY);
    if (rb) {
        rb->SetMass(mass);
        rb->Start();
    }

    return go;
}

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

    mesh.indices = {
        0, 1, 2,  2, 3, 0,
        4, 5, 6,  6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
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

    // Bottom face (base)
    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });

    mesh.vertices.push_back({ {-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ {-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });
    mesh.vertices.push_back({ { 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });

    for (unsigned int i = 0; i < 18; i++)
        mesh.indices.push_back(i);

    return mesh;
}

Mesh Primitives::CreatePlane(float width, float height)
{
    Mesh mesh;

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    // Quad (top)
    mesh.vertices.push_back({ {-halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { halfW, 0.0f, -halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-halfW, 0.0f,  halfH}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });

    // Quad (bottom) with flipped normals
    mesh.vertices.push_back({ {-halfW, 0.0f, -halfH}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
    mesh.vertices.push_back({ { halfW, 0.0f, -halfH}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
    mesh.vertices.push_back({ { halfW, 0.0f,  halfH}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
    mesh.vertices.push_back({ {-halfW, 0.0f,  halfH}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });

    mesh.indices = {
        0, 1, 2,  2, 3, 0,
        4, 6, 5,  6, 4, 7
    };

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

    // Indices arranged so outward-facing normals are preserved
    for (unsigned int r = 0; r < rings - 1; r++)
    {
        for (unsigned int s = 0; s < sectors - 1; s++)
        {
            mesh.indices.push_back(r * sectors + s);
            mesh.indices.push_back((r + 1) * sectors + (s + 1));
            mesh.indices.push_back(r * sectors + (s + 1));

            mesh.indices.push_back(r * sectors + s);
            mesh.indices.push_back((r + 1) * sectors + s);
            mesh.indices.push_back((r + 1) * sectors + (s + 1));
        }
    }

    return mesh;
}

Mesh Primitives::CreateCylinder(float radius, float height, unsigned int segments)
{
    Mesh mesh;
    float halfHeight = height * 0.5f;

    // Body vertices (pairs: top then bottom for each segment)
    for (unsigned int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cosf(angle);
        float z = sinf(angle);
        float u = (float)i / segments;

        // Top vertex
        mesh.vertices.push_back({
            {x * radius, halfHeight, z * radius},
            {x, 0.0f, z},
            {u, 1.0f}
            });

        // Bottom vertex
        mesh.vertices.push_back({
            {x * radius, -halfHeight, z * radius},
            {x, 0.0f, z},
            {u, 0.0f}
            });
    }

    // Side indices
    for (unsigned int i = 0; i < segments; i++)
    {
        unsigned int topCurrent = i * 2;
        unsigned int bottomCurrent = i * 2 + 1;
        unsigned int topNext = (i + 1) * 2;
        unsigned int bottomNext = (i + 1) * 2 + 1;

        mesh.indices.push_back(topCurrent);
        mesh.indices.push_back(topNext);
        mesh.indices.push_back(bottomCurrent);

        mesh.indices.push_back(topNext);
        mesh.indices.push_back(bottomNext);
        mesh.indices.push_back(bottomCurrent);
    }

    unsigned int baseIndex = mesh.vertices.size();

    // Top cap center
    mesh.vertices.push_back({ {0.0f, halfHeight, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });

    // Top cap rim vertices
    for (unsigned int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        mesh.vertices.push_back({ {x, halfHeight, z}, {0.0f, 1.0f, 0.0f}, {0.5f + cosf(angle) * 0.5f, 0.5f + sinf(angle) * 0.5f} });
    }

    // Top cap indices (fan)
    for (unsigned int i = 0; i < segments; i++)
    {
        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 1 + (i + 1));
        mesh.indices.push_back(baseIndex + 1 + i);
    }

    unsigned int bottomBaseIndex = mesh.vertices.size();

    // Bottom cap center
    mesh.vertices.push_back({ {0.0f, -halfHeight, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });

    // Bottom cap rim vertices
    for (unsigned int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;
        mesh.vertices.push_back({ {x, -halfHeight, z}, {0.0f, -1.0f, 0.0f}, {0.5f + cosf(angle) * 0.5f, 0.5f + sinf(angle) * 0.5f} });
    }

    // Bottom cap indices (fan)
    for (unsigned int i = 0; i < segments; i++)
    {
        mesh.indices.push_back(bottomBaseIndex);
        mesh.indices.push_back(bottomBaseIndex + 1 + i);
        mesh.indices.push_back(bottomBaseIndex + 1 + (i + 1));
    }

    return mesh;
}
