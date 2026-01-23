#pragma once
#include "Module.h"
#include <string>

class UIModule : public Module
{
public:
    UIModule();
    virtual ~UIModule();

    bool Start() override;
    bool Update() override;

private:
    void DrawEngineSettings();
    void RenderHUD(); 

private:

    bool isSettingsVisible = false; 
    bool vsyncActive = true;             


    float crosshairSize = 12.0f;
    float crosshairThickness = 2.0f;

    char userTag[64] = "Player 1";  
};