#pragma once
#include "Module.h"
#include <SDL3/SDL_video.h>  
#include <iostream>
#include "FileSystem.h"

class OpenGL : public Module
{
public:
    OpenGL();
    ~OpenGL();

    bool Start() override;
    bool Update() override;
    bool CleanUp() override;

    unsigned int CreateTriangle();
    unsigned int CreateCube(); 
    unsigned int CreatePyramid();

    void LoadMesh(Mesh& mesh);
    void DrawMesh(const Mesh& mesh);
private:

    SDL_GLContext glContext;
    unsigned int shaderProgram;
    Uint VAO_Triangle, VAO_Cube, VAO_Pyramid, VAO_Cylinder;
    Uint VBO, EBO;

    // EBO = Element buffer object

};