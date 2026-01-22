#pragma once
#include "FileSystem.h"

class Primitives
{
public:
    static Mesh CreateTriangle();
    static Mesh CreateCube();
    static Mesh CreatePyramid();
    static Mesh CreatePlane(float width = 1.0f, float height = 1.0f);
    static Mesh CreateSphere(float radius = 0.5f, unsigned int rings = 16, unsigned int sectors = 16);
    static Mesh CreateCylinder(float radius = 0.5f, float height = 1.0f, unsigned int segments = 16);

    static GameObject* CreateCubeGameObject(const std::string& name, float mass = 1.0f);
    static GameObject* CreateSphereGameObject(const std::string& name, float mass = 1.0f);
};