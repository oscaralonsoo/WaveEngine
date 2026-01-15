#include "ModuleUI.h"
#include "Application.h"
#include "ModuleResources.h"
#include "Texture.h"
#include "Input.h"
#include "Window.h"
#include "Time.h"
#include "Log.h"
#include <imgui.h>

ModuleUI::ModuleUI() : Module()
{
    name = "ModuleUI";
}

ModuleUI::~ModuleUI()
{
}

bool ModuleUI::Start()
{
    LOG_DEBUG("Initializing Module UI");
    // AQUÍ PUEDES CARGAR TEXTURAS SI TIENES EL PATH CORRECTO
    // mainMenuBackground = ...
    // crosshairTexture = ...
    return true;
}

bool ModuleUI::Update()
{
    // Si NO estamos jugando (estamos editando), no dibujamos nada de esto
    if (Application::GetInstance().GetPlayState() != Application::PlayState::PLAYING)
        return true;

    // Tecla F1 para abrir/cerrar opciones
    if (Application::GetInstance().input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
    {
        showOptions = !showOptions;
    }

    // Máquina de estados
    switch (uiState)
    {
    case UIState::MAIN_MENU:
        RenderMainMenu();
        break;

    case UIState::FADING_OUT:
        RenderMainMenu(); // Seguimos dibujando el fondo debajo

        // Aumentar alpha para el efecto fade
        fadeAlpha += fadeSpeed * Application::GetInstance().time->GetRealDeltaTime();

        // Dibujar pantalla negra encima con transparencia
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::SetNextWindowBgAlpha(fadeAlpha);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 1)); // Negro
            ImGui::Begin("FadeOverlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::End();
            ImGui::PopStyleColor();
        }

        if (fadeAlpha >= 1.0f)
        {
            uiState = UIState::IN_GAME;
            fadeAlpha = 0.0f;
        }
        break;

    case UIState::IN_GAME:
        RenderHUD();
        if (showOptions) RenderOptionsWindow();
        break;
    }

    return true;
}

void ModuleUI::RenderMainMenu()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    // Ventana sin bordes, ni titulo, fija
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("MainMenu", nullptr, flags);

    // 1. DIBUJAR FONDO
    if (mainMenuBackground)
    {
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::Image((ImTextureID)(unsigned long long)mainMenuBackground->GetID(), viewport->Size);
    }
    else
    {
        // Fondo azul oscuro si no hay imagen
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), viewport->Size, IM_COL32(20, 30, 50, 255));
    }

    // 2. INTERFAZ
    ImVec2 center = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);

    // Titulo
    ImGui::SetCursorPos(ImVec2(center.x - 80, center.y - 100));
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("MI JUEGO");
    ImGui::SetWindowFontScale(1.0f);

    // Input Box
    ImGui::SetCursorPos(ImVec2(center.x - 100, center.y));
    ImGui::Text("Nombre del Jugador:");
    ImGui::SetCursorPosX(center.x - 100);
    ImGui::InputText("##NameInput", playerNameInput, sizeof(playerNameInput));

    // Boton Start
    ImGui::SetCursorPos(ImVec2(center.x - 60, center.y + 60));
    if (ImGui::Button("START GAME", ImVec2(120, 40)))
    {
        StartGame();
    }

    ImGui::End();
}

void ModuleUI::StartGame()
{
    uiState = UIState::FADING_OUT;
    fadeAlpha = 0.0f;
    LOG_CONSOLE("Juego iniciado para: %s", playerNameInput);
}

void ModuleUI::RenderOptionsWindow()
{
    // Ventana draggable normal
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Game Options (F1)", &showOptions))
    {
        ImGui::Text("Opciones Generales");
        ImGui::Separator();

        if (ImGui::Checkbox("Activar VSync", &vsyncEnabled))
        {
            Application::GetInstance().window->SetVsync(vsyncEnabled);
        }

        ImGui::Spacing();
        ImGui::Text("Jugador actual: %s", playerNameInput);

        if (ImGui::Button("Cerrar"))
        {
            showOptions = false;
        }
    }
    ImGui::End();
}

void ModuleUI::RenderHUD()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 center = ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f);

    // Ventana transparente overlay
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // Transparente
    ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Dibujar cruz
    float size = 10.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Linea horizontal y vertical blancas
    drawList->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), IM_COL32(255, 255, 255, 255), 2.0f);
    drawList->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), IM_COL32(255, 255, 255, 255), 2.0f);

    ImGui::End();
    ImGui::PopStyleColor();
}