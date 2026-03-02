#pragma once
#include "Module.h"
#include <SDL3/SDL_video.h>

class RenderContext : public Module
{
public:
    RenderContext();
    ~RenderContext();

    bool Start() override;
    bool Update() override;
    bool PreUpdate() override;
    bool CleanUp() override;

    SDL_GLContext GetContext() const { return glContext; }

private:
    SDL_GLContext glContext;
};