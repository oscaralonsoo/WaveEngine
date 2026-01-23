#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_error.h> 
#include <SDL3/SDL.H>
#include "Module.h"

class Window : public Module
{
public:
    Window();
    ~Window();

    bool Start() override; 
    bool Update() override;
    void Render();
    bool PostUpdate() override;
    bool CleanUp() override;
    void GetWindowSize(int& width, int& height) const;
    int GetScale() const;
    void SetVsync(bool enabled);

    SDL_Window* GetWindow() const { return window; }

private:
    SDL_Window* window;

    int width;
    int height;
    int scale;
};