#pragma once
#include <SDL3/SDL.h>
#include "Module.h"

class Window : public Module
{
public:
    Window();
    virtual ~Window();

    bool Start() override; 

    bool Update() override;

    bool PostUpdate() override;

    bool CleanUp() override;

    void SetVsync(bool enabled);
    bool GetVsync() const { return vsyncActive; }

    void GetWindowSize(int& width, int& height) const;
    int GetScale() const;

    SDL_Window* GetWindow() const { return window; }

private:
    void Render();

private:
    SDL_Window* window = nullptr;

    int width = 1280;
    int height = 720;
    int scale = 1;

    bool vsyncActive = true;
};