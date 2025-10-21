#include "RenderContext.h"
#include "RenderContext.h"
#include "Application.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>

RenderContext::RenderContext() : glContext(nullptr)
{
    std::cout << "RenderContext Constructor" << std::endl;
}

RenderContext::~RenderContext()
{
}

bool RenderContext::Start()
{
    std::cout << "Init OpenGL Context & GLAD" << std::endl;

    SDL_Window* window = Application::GetInstance().window->GetWindow();
    glContext = SDL_GL_CreateContext(window);

    if (!glContext)
    {
        std::cerr << "Failed to create OpenGL context" << std::endl;
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::cout << "OpenGL Context initialized successfully" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return true;
}

bool RenderContext::PreUpdate()
{
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

bool RenderContext::Update()
{
    return true;
}

bool RenderContext::CleanUp()
{
    std::cout << "Destroying OpenGL Context" << std::endl;

    if (glContext != nullptr)
    {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }

    return true;
}