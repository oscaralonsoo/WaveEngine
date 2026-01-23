#pragma once

#include <SDL3/SDL.h>
#include "Module.h"

class Window : public Module
{
public:
    Window();
    ~Window() override;

    bool Start() override;
    bool Update() override;
    bool PostUpdate() override;
    bool CleanUp() override;

    void SetVsync(bool enabled);
    bool IsVsyncActive() const { return vsyncActive; }

    void GetWindowSize(int& width, int& height) const;
    SDL_Window* GetWindow() const { return window; }

private:
    void Render();

private:
    SDL_Window* window = nullptr;
    int width;
    int height;
    int scale;

    bool vsyncActive = true; 
};