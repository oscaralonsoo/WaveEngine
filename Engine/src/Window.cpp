#include "Window.h"
#include <glad/glad.h>
#include "Log.h"

Window::Window() : window(nullptr), width(1280), height(720), scale(1), vsyncActive(true)
{
}

Window::~Window() {}

bool Window::Start()
{
    LOG_CONSOLE("Window Module: Initializing for Version 3...");

    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(
        "Wave Engine - V3 Build",
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) return false;

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SetVsync(vsyncActive);

    LOG_CONSOLE("Window created successfully (1280x720)");
    return true;
}

void Window::SetVsync(bool enabled)
{
    vsyncActive = enabled;
    if (SDL_GL_SetSwapInterval(vsyncActive ? 1 : 0) < 0)
    {
        LOG_DEBUG("Warning: SDL could not set VSync! Error: %s", SDL_GetError());
    }
}

bool Window::Update()
{
    int newWidth, newHeight;
    SDL_GetWindowSize(window, &newWidth, &newHeight);

    if (newWidth != width || newHeight != height)
    {
        width = newWidth;
        height = newHeight;
        glViewport(0, 0, width, height);
    }
    return true;
}

bool Window::PostUpdate()
{
    SDL_GL_SwapWindow(window);
    return true;
}

bool Window::CleanUp()
{
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}

void Window::GetWindowSize(int& w, int& h) const { w = width; h = height; }