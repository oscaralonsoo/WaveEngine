#pragma once
#include "Module.h"

class Grid : public Module
{
public:
    Grid();
    ~Grid();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    void Draw();

    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool state) { enabled = state; }

private:

    unsigned int VAO;
    unsigned int VBO;
    int numVertices;

    float gridSize;
    int gridDivisions;
    bool enabled;
};