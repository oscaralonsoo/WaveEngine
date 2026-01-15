#pragma once
#include "Module.h"
#include <string>
#include <memory>

// Forward declarations
class Texture;

class ModuleUI : public Module
{
public:
    ModuleUI();
    ~ModuleUI();

    bool Start() override;
    bool Update() override;

    // Estados de la interfaz
    enum class UIState {
        MAIN_MENU,
        FADING_OUT,
        IN_GAME
    };

private:
    void RenderMainMenu();
    void RenderHUD();           // Crosshair
    void RenderOptionsWindow(); // Ventana F1

    void StartGame();

private:
    UIState uiState = UIState::MAIN_MENU;

    // Texturas (opcional, si no cargan se usará color)
    std::shared_ptr<Texture> mainMenuBackground;
    std::shared_ptr<Texture> crosshairTexture;

    // Variables del Menú Principal
    char playerNameInput[64] = "Player 1";
    float fadeAlpha = 0.0f;
    float fadeSpeed = 0.5f; // Velocidad del fundido a negro

    // Variables de Opciones (F1)
    bool showOptions = false;
    bool vsyncEnabled = true;
};