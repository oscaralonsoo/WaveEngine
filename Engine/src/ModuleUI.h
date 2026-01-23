#pragma once
#include "Module.h"
#include <string>

enum class UIState {
    MAIN_MENU,
    FADING,
    GAME_HUD
};

class ModuleUI : public Module {
public:
    ModuleUI();
    virtual ~ModuleUI() = default;

    bool Start() override;
    bool Update() override;
    bool PostUpdate() override;

    void ResetUI();      
    void SaveConfig();   
    void LoadConfig(); 

private:
    void DrawMainMenu();
    void DrawFade();

private:
    UIState currentState = UIState::MAIN_MENU;
    std::string userName;
    char nameBuffer[64] = ""; 

    float fadeAlpha = 0.0f;  
    float fadeSpeed = 1.2f;   
    unsigned int backgroundTextureID = 0;
};