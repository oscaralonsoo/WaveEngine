#pragma once
#include "Module.h"
#include <string>
#include <memory>

class Texture;

class UIModule : public Module
{
public:
    UIModule();
    ~UIModule();

    bool Start() override;
    bool Update() override;

    enum class UIState {
        MAIN_MENU,
        FADING_OUT,
        IN_GAME
    };
   static bool colour;
    static void DrawCrosshairInsideWindow();
    
private:
    void RenderMainMenu();
    void RenderHUD();
    void RenderOptionsWindow();

    void StartGame();

private:
    UIState uiState = UIState::MAIN_MENU;
    std::shared_ptr<Texture> mainMenuBackground;
    std::shared_ptr<Texture> crosshairTexture;

    char playerNameInput[64] = "Player 1";
    float fadeAlpha = 0.0f;
    float fadeSpeed = 0.5f; 

    bool showOptions = false;
    bool vsyncEnabled = true;
    
};