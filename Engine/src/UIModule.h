#pragma once
#include "Module.h"
#include <string>

class ModuleUI : public Module
{
public:
    ModuleUI();
    ~ModuleUI() override;

    bool Start() override;
    bool Update() override;


    enum class UIState {
        MAIN_MENU,
        FADING_OUT,
        IN_GAME
    };

private:
    void DrawEngineSettings();

private:
    UIState uiState = UIState::IN_GAME;
    

    bool isSettingsVisible = false; 
    bool vsyncActive = true;        
     

    char userTag[64] = "Player 1";  
};