#pragma once
#include "Module.h"
#include <SDL3/SDL.h> //Buscar una mas concreta para usar SDL_GLContext
struct SDL_Window;

class OpenGL : public Module
{
public:
	OpenGL();
	~OpenGL();

	SDL_GLContext glContext;
	unsigned int shaderProgram;
	unsigned int VAO;
	unsigned int VBO;

	//Revisar porque esta publico o privado

private:
	bool Start() override;
	bool CleanUp() override;
	bool Update() override;
};