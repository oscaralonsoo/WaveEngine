#pragma once
#include "Module.h"
#include "FileSystem.h"
#include "Shaders.h"
#include <memory>
#include "Primitives.h"

class Renderer : public Module
{
public:
    Renderer();
    ~Renderer();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    void LoadMesh(Mesh& mesh);
    void DrawMesh(const Mesh& mesh);
    void UnloadMesh(Mesh& mesh);

    Shader* GetDefaultShader() const { return defaultShader.get(); }

private:

    std::unique_ptr<Shader> defaultShader;
    Mesh sphere, cube, pyramid, cylinder, plane;
};